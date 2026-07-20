from typing import NamedTuple, List, Mapping, Optional, Tuple, Set, DefaultDict
from collections import defaultdict
import pickle
import argparse

import os
import io
import re

import pcpp
from pycparser import c_ast, CParser

g_failed = False

def verbose(msg):
    print(msg, flush=True)

def error(msg):
    global g_failed
    print(f"\n{msg}", flush=True)
    g_failed = True

fake_includes = { }

fake_includes["stdint.h"] = """
#pragma once
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long int64_t;
"""

fake_includes["stdbool.h"] = """
#pragma once
#define bool _Bool
#define true 1
#define false 0
"""

fake_includes["stddef.h"] = """
typedef unsigned long size_t;
typedef unsigned long uintptr_t;
typedef long ptrdiff_t;
"""

fake_includes["stdarg.h"] = """
typedef void *va_list;
#define va_arg(a, b) (*(b*)(a))
"""

fake_includes["stdio.h"] = """
typedef int FILE;
typedef unsigned long fpos_t;
"""

fake_includes["string.h"] = ""
fake_includes["stdlib.h"] = ""
fake_includes["locale.h"] = ""
fake_includes["math.h"] = ""
fake_includes["assert.h"] = ""
fake_includes["float.h"] = ""

class Preprocessor(pcpp.Preprocessor):
    def __init__(self):
        super().__init__()
    
    def on_file_open(self, is_system_include, includepath):
        if is_system_include and os.path.basename(includepath) in fake_includes:
            return io.StringIO(fake_includes[os.path.basename(includepath)])
        return super().on_file_open(is_system_include, includepath)

class ValuePreprocessor(pcpp.Preprocessor):
    def __init__(self, func: str, expr: str):
        super().__init__()
        self.func = func
        self.expr = expr
    
    def on_unknown_macro_in_expr(self,tok):
        raise RuntimeError(f"Bad recursion bound on function {self.func}: '{self.expr}'")

class FuncInfo:
    stack_usage: int
    max_recursion: Optional[int]
    calls: Set[str]
    defined: bool
    recursion_stack: List[str]

    def __init__(self):
        self.stack_usage = 0
        self.max_recursion = None
        self.calls = set()
        self.defined = False
        self.recursion_stack = []
        self.largest_call = ""

class StackUsage(NamedTuple):
    usage: int
    largest_call: str

class File:
    functions: DefaultDict[str, FuncInfo]
    stack_usage: Mapping[str, StackUsage]
    addresses: Set[str]
    max_dynamic_usage: int
    recursion_errors: Mapping[str, List[str]]

    def __init__(self):
        self.functions = defaultdict(FuncInfo)
        self.stack_usage = {}
        self.addresses = set()
        self.max_dynamic_usage = 0
        self.recursion_errors = {}

class AstVisitor(c_ast.NodeVisitor):
    file: File
    current_func: str

    def __init__(self, file: File):
        self.file = file
        self.current_func = None

    def visit_FuncDef(self, node: c_ast.FuncDecl):
        if node.body:
            func = node.decl.name
            if not (func.startswith("ufbx_") or func.startswith("ufbxi_")):
                error(f"Bad function definition: {func}")
            self.file.functions[func].defined = True

            self.current_func = node.decl.name
            self.visit(node.body)
            self.current_func = None

    def visit_UnaryOp(self, node: c_ast.UnaryOp):
        if node.op == "&" and isinstance(node.expr, c_ast.ID):
            self.file.addresses.add(node.expr.name)
        self.visit(node.expr)

    def visit_FuncCall(self, node: c_ast.FuncCall):
        src = self.current_func
        if src:
            dst = node.name.name
            if (isinstance(dst, str) and (dst.startswith("ufbxi_") or dst.startswith("ufbx_"))):
                self.file.functions[src].calls.add(dst)
        if node.args:
            self.visit(node.args)

def get_stack_usage_to(file: File, func: str, target: str, seen: Set[str] = set()) -> Optional[Tuple[int, List[str]]]:
    if func in seen:
        return (0, []) if func == target else None
    info = file.functions.get(func)
    if not info:
        raise RuntimeError(f"Function not found: {func}")

    seen = seen | { func }

    max_path = None
    for call in info.calls:
        path = get_stack_usage_to(file, call, target, seen)
        if path is not None:
            if max_path:
                max_path = max(max_path, path)
            else:
                max_path = path

    if max_path:
        max_usage, stack = max_path
        usage = info.stack_usage + max_usage
        return (usage, [func] + stack)
    else:
        return None

def add_ignore(ignores: str, ignore: str) -> str:
    if not ignores: ignores = "/"
    parts = ignores[1:].split(",") + [ignore]
    return "/" + ",".join(sorted(p for p in parts if p))

def is_ignored(ignores: str, func: str) -> bool:
    return ignores and func in ignores[1:].split(",")

def get_stack_usage(file: File, func: str, ignores: str = "", stack: List[str] = []) -> StackUsage:
    if is_ignored(ignores, func):
        return StackUsage(-1, "")

    key = f"{func}{ignores}" if ignores else func
    existing = file.stack_usage.get(key)
    if existing is not None: return existing

    info = file.functions.get(func)
    if not info:
        raise RuntimeError(f"Function not found: {func}")

    if info.max_recursion:
        rec_path = get_stack_usage_to(file, func, func)
        if rec_path is None:
            error(f"Unnecessary recursion tag in {func}()\nContains ufbxi_assert_max_recursion() but could not find recursive path to itself")
            rec_path = (0, [])

        rec_usage, rec_stack = rec_path
        info.recursion_stack = rec_stack
        self_usage = rec_usage * (info.max_recursion - 1) + info.stack_usage
        child_ignores = add_ignore(ignores, func)
        stack = []
    else:
        self_usage = info.stack_usage
        child_ignores = ignores

    if func in stack:
        pos = stack.index(func)
        error_stack = stack[pos:] + [func]
        prev_error = file.recursion_errors.get(func)
        if not prev_error or len(prev_error) > len(error_stack):
            file.recursion_errors[func] = error_stack
        return StackUsage(0, "")

    stack = stack + [func]

    max_usage = StackUsage(0, "")
    for call in info.calls:
        usage = get_stack_usage(file, call, child_ignores, stack).usage
        max_usage = max(max_usage, StackUsage(usage, f"{call}{child_ignores}"))

    usage = StackUsage(self_usage + max_usage.usage, max_usage.largest_call)
    file.stack_usage[key] = usage
    return usage

def parse_file(c_path: str, su_path: str, cache_path: Optional[str]) -> File:
    pp = Preprocessor()
    pp.define("UFBX_STANDARD_C")
    pp.define("UFBXI_ANALYSIS_PARSER")
    pp.define("UFBXI_ANALYSIS_RECURSIVE")

    if cache_path and os.path.exists(cache_path) and os.path.getmtime(cache_path) > os.path.getmtime(c_path):
        verbose(f"Loading AST cache: {cache_path}")
        with open(cache_path, "rb") as f:
            ast, max_recursions = pickle.load(f)
    else:
        max_recursions = { }

        verbose(f"Preprocessing C file: {c_path}")
        pp_stream = io.StringIO()
        with open(c_path) as f:
            pp.parse(f.read(), "ufbx.c")
            pp.write(pp_stream)
        pp_source = pp_stream.getvalue()

        re_recursive_function = re.compile(r"UFBXI_RECURSIVE_FUNCTION\s*\(\s*(\w+),\s*(.+)\s*\);")
        for line in pp_source.splitlines():
            m = re_recursive_function.search(line)
            if m:
                name, rec_expr = m.groups()
                lit_pp = ValuePreprocessor(name, rec_expr)
                toks = lit_pp.tokenize(rec_expr)
                rec_value, _ = lit_pp.evalexpr(toks)
                if not rec_value:
                    raise RuntimeError(f"Bad recursion bound on function {name}: '{rec_expr}'")
                max_recursions[name] = rec_value

        pp_source = re_recursive_function.sub("", pp_source)

        verbose("Parsing C file")
        parser = CParser()
        ast = parser.parse(pp_source)

        if cache_path:
            verbose(f"Writing AST cache: {cache_path}")
            with open(cache_path, "wb") as f:
                pickle.dump((ast, max_recursions), f)

    verbose("Visiting AST")
    file = File()
    visitor = AstVisitor(file)
    visitor.visit(ast)

    # Gather maximum recursion from file
    for func, rec in max_recursions.items():
        file.functions[func].max_recursion = rec

    if su_path:
        verbose(f"Reading stack usage file: {su_path}")
        with open(su_path) as f:
            for line in f:
                line = line.strip()
                if not line: continue
                m = re.match(r".*:\d+:(?:\d+:)?(\w+)(?:\.[a-z0-9\.]+)?\s+(\d+)\s+([a-z,]+)", line)
                if not m:
                    raise RuntimeError(f"Bad .su line: {line}")
                func, stack, usage = m.groups()
                assert usage in ("static", "dynamic,bounded")
                file.functions[func].stack_usage = int(stack)

    return file

def get_max_dynamic_usage(file: File) -> Tuple[int, str]:
    max_dynamic_usage = (0, "")

    addr_funcs = file.addresses & set(file.functions.keys())
    for func in addr_funcs:
        usage = get_stack_usage(file, func).usage
        max_dynamic_usage = max(max_dynamic_usage, (usage, func))

    return max_dynamic_usage

def dump_largest_stack(file: File, func: str) -> List[str]:
    usage = file.stack_usage[func]
    print(f"{func}() {usage.usage} bytes")
    index = 0
    while True:
        if "/" in func:
            raw_func, _ = func.split("/")
        else:
            raw_func = func
        info = file.functions.get(raw_func)
        stack_usage = file.stack_usage.get(func)
        if not info: break

        if info.recursion_stack:
            rec_usage = 0
            for ix, frame in enumerate(info.recursion_stack):
                rec_info = file.functions[frame]
                ch = "|" if ix > 0 else ">"
                fn = f"{frame}()"
                usage = f"+{rec_info.stack_usage} bytes"
                rec_usage += rec_info.stack_usage
                print(f"{ch}{index:3} {fn:<40}{usage:>14}")
                index += 1

            rec_extra = info.max_recursion - 1
            usage = f"(+{rec_usage * rec_extra} bytes)"
            prefix = f"(recursion {info.max_recursion} times)"
            index += rec_extra * len(info.recursion_stack)
            dots = "." * len(str(index - 1))
            print(f"|{dots:>3} {prefix:<39}{usage:>16}")

        fn = f"{raw_func}()"
        usage = f"+{info.stack_usage} bytes"
        print(f"{index:4} {fn:<40}{usage:>14}")
        func = stack_usage.largest_call
        index += 1

if __name__ == "__main__":
    parser = argparse.ArgumentParser("analyze_stack.py")
    parser.add_argument("su", nargs="?", help="Stack usage .su file")
    parser.add_argument("--source", help="Path to ufbx.c")
    parser.add_argument("--cache", help="Cache file to use")
    parser.add_argument("--limit", help="Maximum stack usage in bytes")
    parser.add_argument("--no-su", action="store_true", help="Allow running with no .su file")
    argv = parser.parse_args()

    if argv.su:
        su_path = argv.su
    elif not argv.no_su:
        raise RuntimeError("Expected an .su file as a positional argument")
    else:
        su_path = ""

    if argv.source:
        c_path = argv.source
    else:
        c_path = os.path.relpath(os.path.join(os.path.dirname(__file__), "..", "ufbx.c"))

    file = parse_file(c_path, su_path, argv.cache)

    if su_path:
        file.max_dynamic_usage = get_max_dynamic_usage(file)

        max_usage = (0, "")
        for func in file.functions:
            usage = get_stack_usage(file, func)
            max_usage = max(max_usage, (usage.usage, func))

        if argv.limit:
            limit = int(argv.limit, base=0)
            total = max_usage[0] + file.max_dynamic_usage[0]
            if total >= limit:
                error(f"Stack overflow in {max_usage[1]}: {max_usage[0]} bytes + {file.max_dynamic_usage[0]} dynamic\noverflows limit of {limit} bytes")
                print("\nLargest stack:")
                dump_largest_stack(file, max_usage[1])
                print("\nLargest dynamic stack:")
                dump_largest_stack(file, file.max_dynamic_usage[1])

        for func, stack in file.recursion_errors.items():
            stack_str = "\n".join(f"{ix:3}: {s}()" for ix, s in enumerate(stack))
            error(f"Unbounded recursion in {func}()\nStack trace:\n{stack_str}")

        if not g_failed:
            interesting_functions = [
                "ufbx_load_file",
                "ufbx_evaluate_scene",
                "ufbx_subdivide_mesh",
                "ufbx_tessellate_nurbs_surface",
                "ufbx_tessellate_nurbs_curve",
                "ufbx_evaluate_transform",
                "ufbx_generate_indices",
                "ufbx_inflate",
                "ufbx_triangulate_face",
            ]

            for func in interesting_functions:
                print()
                dump_largest_stack(file, func)

            print()
            print("Largest potentially dynamically called stack:")
            dump_largest_stack(file, file.max_dynamic_usage[1])
    else:
        print("Skipping further tests due to no .su file specified")

    if g_failed:
        exit(1)
    else:
        print()
        print("Success!")
