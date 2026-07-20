#!/usr/bin/env python3

import asyncio
import itertools
import subprocess
import time
import re
import os
import sys
import shutil
import functools
import argparse
import copy
from typing import NamedTuple
from get_header_tag import get_ufbx_header_version

parser = argparse.ArgumentParser(description="Run ufbx tests")
parser.add_argument("tests", type=str, nargs="*", help="Names of tests to run")
parser.add_argument("--additional-compiler", default=[], action="append", help="Additional compiler(s) to use")
parser.add_argument("--remove-compiler", default=[], action="append", help="Remove a compiler from being used")
parser.add_argument("--remove-arch", default=[], action="append", help="Remove an architecture")
parser.add_argument("--compiler", default=[], action="append", help="Specify exact compiler(s) to use")
parser.add_argument("--define", default=[], action="append", help="Define preprocessor symbols")
parser.add_argument("--no-sse", action="store_true", help="Don't make SSE builds when possible")
parser.add_argument("--wasi-sdk", default=[], action="append", help="WASI SDK path")
parser.add_argument("--wasm-runtime", default="wasmtime", type=str, help="WASM runtime command")
parser.add_argument("--no-sanitize", action="store_true", help="Don't compile builds with ASAN/UBSAN")
parser.add_argument("--no-sanitize-arch", default=[], action="append", help="Disable sanitization on specific architectures")
parser.add_argument("--threads", type=int, default=0, help="Number of threads to use (0 to autodetect)")
parser.add_argument("--verbose", action="store_true", help="Verbose output")
parser.add_argument("--hash-file", help="Hash test input file")
parser.add_argument("--runner", help="Descriptive name for the runner")
parser.add_argument("--heavy", action="store_true", help="Run heavy tests")
parser.add_argument("--strict", action="store_true", help="Require strict checks")
parser.add_argument("--hash-threads", action="store_true", help="Use threading for hashes")
parser.add_argument("--fail-on-pre-test", action="store_true", help="Indicate failure if pre-test checks fail")
parser.add_argument("--force-opt", help="Force compiler optimization level")
argv = parser.parse_args()

color_out = sys.stdout

has_color = False
if sys.platform == "win32":
    try:
        import colorama
        colorama.init(wrap=False)
        color_out = colorama.AnsiToWin32(color_out).stream
        has_color = True
    except ImportError:
        print("# Run 'pip install colorama' for colorful output")
else:
    has_color = sys.stdout.isatty()

STYLE_FAIL = "\x1b[31m"
STYLE_WARN = "\x1b[33m"
STYLE_CMD = "\x1b[36m"

def log(line, *, style=""):
    if style and has_color:
        line = style + line + "\x1b[39m"
    print(line, file=color_out, flush=True)

def log_cmd(line):
    log(line, style=STYLE_CMD)

def log_mkdir(path):
    log_cmd("mkdir " + path)

def log_comment(line, fail=False, warn=False):
    style = ""
    if fail: style = STYLE_FAIL
    if warn: style = STYLE_WARN
    if sys.platform == "win32":
        log("rem " + line, style=style)
    else:
        log("# " + line, style=style)

def flatten_str_list(str_list):
    """Flatten arbitrarily nested str list `item` to `dst`"""
    def inner(result, str_list):
        if isinstance(str_list, str):
            result.append(str_list)
        else:
            for s in str_list:
                inner(result, s)
    result = []
    inner(result, str_list)
    return result

if argv.threads > 0:
    num_threads = argv.threads
else:
    num_threads = os.cpu_count()

g_cmd_sema = None
def get_cmd_sema():
    global g_cmd_sema
    if not g_cmd_sema:
        g_cmd_sema = asyncio.Semaphore(num_threads)
    return g_cmd_sema

async def run_cmd(*args, realtime_output=False, env=None, cwd=None):
    """Asynchronously run a command"""

    cmd_sema = get_cmd_sema()
    await cmd_sema.acquire()

    cmd_args = flatten_str_list(args)
    cmd = cmd_args[0]
    cmd_args = cmd_args[1:]

    exec_cmd = cmd
    if cwd:
        line_cmd = os.path.relpath(cmd, cwd)
        exec_cmd = os.path.abspath(cmd)

        cmdline = subprocess.list2cmdline([line_cmd] + cmd_args)
        if sys.platform == "win32":
            cmdline = f"pushd {cwd} & {cmdline} & popd"
        else:
            cmdline = f"pushd {cwd} ; {cmdline} ; popd"
    else:
        cmdline = subprocess.list2cmdline([cmd] + cmd_args)

    pipe = None if realtime_output else asyncio.subprocess.PIPE

    out = err = ""
    ok = False

    log_cmd(cmdline)

    begin = time.time()

    try:
        proc = await asyncio.create_subprocess_exec(exec_cmd, *cmd_args,
            stdout=pipe, stderr=pipe, env=env, cwd=cwd)

        if not realtime_output:
            out, err = await proc.communicate()
            out = out.decode("utf-8", errors="ignore").strip()
            err = err.decode("utf-8", errors="ignore").strip()

        ok = proc.returncode == 0
    except FileNotFoundError:
        err = f"{cmd} not found"
    except OSError as e:
        err = str(e)

    end = time.time()

    cmd_sema.release()

    return ok, out, err, cmdline, end - begin

def config_fmt_arch(config):
    arch = config["arch"]
    if config.get("san"):
        arch += "-san"
    return arch

async def run_fail(message):
    return False, "", "CL: asan not supported", "", 0.0

class Compiler:
    def __init__(self, name, exe):
        self.name = name
        self.exe = exe
        self.env = None
        self.compile_archs = set()
        self.run_archs = set()

    def run(self, *args, **kwargs):
        return run_cmd(self.exe, args, env=self.env, **kwargs)

class CLCompiler(Compiler):
    def __init__(self, name, exe):
        super().__init__(name, exe)
        self.has_c = True
        self.has_cpp = True

    async def check_version(self):
        _, out, err, _, _ = await self.run()
        m = re.search(r"Version ([.0-9]+) for (\w+)", out + err, flags=re.M)
        if not m: return False
        self.arch = m.group(2).lower()
        self.version = m.group(1)
        return True

    def supported_archs(self):
        if self.arch == "x86":
            return ["x86"]
        if self.arch == "x64":
            return ["x64"]
        return []

    def compile(self, config):
        if config.get("asan"):
            return run_fail("CL: asan not supported")
        if config.get("ubsan"):
            return run_fail("CL: ubsan not supported")

        sources = config["sources"]
        output = config["output"]

        args = []
        args += sources
        args += ["/MT", "/nologo"]

        if not config.get("compile_only"):
            obj_dir = os.path.dirname(output)
            if "temp_dir" in config:
                obj_dir = config["temp_dir"]
            args.append(f"/Fo{obj_dir}\\")

        if config.get("warnings", False):
            args.append("/W4")
            args.append("/WX")
        else:
            args.append("/W3")

        if config.get("compile_only"):
            args.append("/c")

        if config.get("dev", True):
            args.append("/DUFBX_DEV")

        if config.get("optimize", False):
            args.append("/Ox")
        else:
            args.append("/DDEBUG=1")

        if config.get("regression", False):
            args.append("/DUFBX_REGRESSION=1")

        std = config.get("std", "")
        if std:
            assert std in ("c++14", "c++17", "c++20")
            args.append(f"/std:{std}")

        for key, val in config.get("defines", {}).items():
            if not val:
                args.append(f"/D{key}")
            else:
                args.append(f"/D{key}={val}")

        if config.get("openmp", False):
            args.append("/openmp")
        
        if config.get("cpp", False):
            args.append("/EHsc")

        if config.get("sse", False):
            args.append("/DSSE=1")

        if config.get("compile_only"):
            args.append(f"/Fo\"{output}\"")
        else:
            args.append("/link")
            args += ["/opt:ref"]
            args.append(f"-out:{output}")

        return self.run(args)

class GCCCompiler(Compiler):
    def __init__(self, name, exe, cpp, has_m32=True):
        super().__init__(name, exe)
        self.has_c = not cpp
        self.has_cpp = cpp
        self.has_m32 = has_m32
        self.sysroot = ""
        self.version_tuple = (0,0,0)

    async def check_version(self):
        _, vout, _, _, _ = await self.run("-dumpversion")
        _, mout, _, _, _ = await self.run("-dumpmachine")
        if not (vout and mout): return False
        self.version = vout.strip()
        self.arch = mout.lower()

        m = re.match(r"^(\d+)\.(\d+)\.(\d+)$", self.version)
        if m:
            self.version_tuple = (int(m.group(1)), int(m.group(2)), int(m.group(3)))
        m = re.match(r"^(\d+)$", self.version)
        if m:
            self.version_tuple = (int(m.group(1)), 0, 0)

        return True

    def supported_archs_raw(self):
        if "x86_64" in self.arch:
            return ["x86", "x64"] if self.has_m32 else ["x64"]
        if "i686" in self.arch:
            return ["x86"]
        if "arm-" in self.arch or "armv7l-" in self.arch or "armv7b-" in self.arch:
            return ["arm32"]
        if "aarch64" in self.arch:
            return ["arm64"]
        if "wasm32" in self.arch:
            return ["wasm32"]
        return []

    def supported_archs(self):
        raw_archs = self.supported_archs_raw()
        archs = [*raw_archs]
        archs += (a + "-san" for a in raw_archs if a != "wasm32")
        return archs

    def modify_compile_args(self, config, args):
        pass

    def compile(self, config):
        sources = config["sources"]
        output = config["output"]

        args = []

        if self.sysroot:
            args += ["--sysroot", self.sysroot]

        if config.get("warnings", False):
            args += ["-Wall", "-Wextra", "-Wsign-conversion"]
            if not self.has_cpp:
                args += ["-Wmissing-prototypes"]

            ufbx_version = get_ufbx_header_version()
            if ufbx_version >= (0, 5, 1):
                args += ["-Wshadow", "-Wundef"]
                if "clang" in self.name:
                    args += [
                        "-Wunreachable-code-break",
                        "-Wmissing-variable-declarations",
                        "-Wfloat-conversion",
                    ]
                    if self.version_tuple >= (8,0,0):
                        args += [
                            "-Wextra-semi-stmt",
                        ]
                    if self.version_tuple >= (10,0,0):
                        args += [
                            "-Wimplicit-int-float-conversion",
                            "-Wimplicit-int-conversion",
                        ]
                elif "gcc" in self.name:
                    args += [
                        "-Wswitch-default",
                        "-Wconversion",
                    ]
                    if self.version_tuple >= (6,0,0):
                        args += [
                            "-Wduplicated-cond",
                        ]
                    if self.version_tuple >= (10,0,0):
                        args += [
                            "-Warith-conversion",
                        ]

            if self.has_cpp:
                args += ["-Wconversion-null"]
            else:
                args += ["-Wstrict-prototypes"]
            args += ["-Werror"]

        args.append("-g")

        if argv.force_opt:
            args.append(f"-{argv.force_opt}")
        else:
            if config.get("optimize", False):
                if config.get("san", False):
                    args.append("-O0")
                else:
                    args.append("-O2")
                args.append("-DNDEBUG=1")

        if config.get("regression", False):
            args.append("-DUFBX_REGRESSION=1")

        if config.get("openmp", False):
            args.append("-openmp")

        if self.has_m32 and config.get("arch", "") == "x86":
            args.append("-m32")

        if config.get("compile_only"):
            args.append("-c")

        if self.has_cpp:
            std = "c++11"
        else:
            std = "gnu99"
        std = config.get("std", std)
        args.append(f"-std={std}")

        if config.get("dev", True):
            args.append("-DUFBX_DEV")

        use_sse = False
        if config.get("sse", False):
            args.append("-DSSE=1")
            args += ["-mbmi", "-msse3", "-msse4.1", "-msse4.2"]
            if config.get("arch", "") == "x86":
                use_sse = True

        if config.get("arch", "") == "x86" and config.get("ieee754", False):
            use_sse = True
            args += ["-mfpmath=sse"]

        if config.get("ieee754", False):
            args += ["-ffp-contract=off"]

        if use_sse:
            args += ["-msse", "-msse2"]

        if config.get("threads", False):
            args.append("-pthread")

        if config.get("stack_protector", False):
            args.append("-fstack-protector")

        if config.get("san"):
            args.append("-fsanitize=address")
            if sys.platform != "darwin":
                args.append("-fsanitize=leak")
            args.append("-fsanitize=undefined")
            args.append("-fno-sanitize=float-cast-overflow")
            args.append("-fno-sanitize-recover=undefined")
            args.append("-DUFBX_UBSAN")

        if "mingw" in self.arch:
            args.append("-D__USE_MINGW_ANSI_STDIO=1")

        for key, val in config.get("defines", {}).items():
            if not val:
                args.append(f"-D{key}")
            else:
                args.append(f"-D{key}={val}")

        args += sources

        if "msvc" not in self.arch and not config.get("compile_only"):
            args.append("-lm")

        args += ["-o", output]

        self.modify_compile_args(config, args)

        return self.run(args)

class ClangCompiler(GCCCompiler):
    def __init__(self, name, exe, cpp, **kwargs):
        super().__init__(name, exe, cpp, **kwargs)

class TCCCompiler(GCCCompiler):
    def __init__(self, name, exe):
        super().__init__(name, exe, False)
        self.has_c = True
        self.has_cpp = False
        self.has_m32 = True
        self.sysroot = ""

    async def check_version(self):
        _, out, err, _, _ = await self.run("-v")
        print(out + err)
        m = re.search(r"tcc version ([.0-9]+) \((\w+).*\)", out + err, flags=re.M)
        if not m: return False
        self.arch = m.group(2).lower()
        self.version = m.group(1)
        return True

    def supported_archs(self):
        if "x86_64" in self.arch:
            return ["x86", "x64"] if self.has_m32 else ["x64"]
        if "i686" in self.arch:
            return ["x86"]
        return []

class EmscriptenCompiler(ClangCompiler):
    def __init__(self, name, exe, cpp):
        super().__init__(name, exe, cpp)

class WasiCompiler(ClangCompiler):
    def __init__(self, name, exe, cpp, sysroot):
        super().__init__(name, exe, cpp)
        self.sysroot = sysroot

    def modify_compile_args(self, config, args):
        args.append("-Wl,--stack-first")

class ZigCompiler(ClangCompiler):
    def __init__(self, name, exe, cpp):
        super().__init__(name, exe, cpp)

@functools.lru_cache(8)
def get_vcvars(bat_name):
    vswhere_path = r"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe"
    vswhere_path = os.path.expandvars(vswhere_path)
    if not os.path.exists(vswhere_path):
        raise EnvironmentError("vswhere.exe not found at: %s", vswhere_path)

    vs_path = os.popen('"{}" -latest -property installationPath'.format(vswhere_path)).read().rstrip()
    vsvars_path = os.path.join(vs_path, f"VC\\Auxiliary\\Build\\{bat_name}")

    output = os.popen('"{}" && set'.format(vsvars_path)).read()
    env = { }
    for line in output.splitlines():
        items = tuple(line.split("=", 1))
        if len(items) == 2:
            env[items[0]] = items[1]
    return env

class VsCompiler(Compiler):
    def __init__(self, name, bat, inner):
        super().__init__(name, inner.exe)
        self.bat = bat
        self.inner = inner

    async def check_version(self):
        try:
            env = get_vcvars(self.bat)
            self.inner.env = env
            for key, value in env.items():
                if key.upper() != "PATH": continue
                for path in value.split(";"):
                    cl_path = os.path.join(path, self.exe)
                    if os.path.exists(cl_path):
                        self.inner.exe = cl_path
                        if await self.inner.check_version():
                            self.exe = self.inner.exe
                            self.arch = self.inner.arch
                            self.version = self.inner.version
                            self.has_c = self.inner.has_c
                            self.has_cpp = self.inner.has_cpp
                            return True
            return False
        except:
            return False

    def supported_archs(self):
        return self.inner.supported_archs()

    def supports_config(self, config):
        return self.inner.supports_config(config)

    def compile(self, config):
        return self.inner.compile(config)

def supported_archs(compiler):
    archs = compiler.supported_archs()
    if argv.no_sanitize:
        archs = [a for a in archs if not a.endswith("-san")]
    if argv.no_sanitize_arch:
        archs = [a for a in archs if not a.endswith("-san") or a[:-4] not in argv.no_sanitize_arch]
    return archs

all_compilers = [
    CLCompiler("cl", "cl.exe"),
    GCCCompiler("gcc", "gcc", False),
    GCCCompiler("gcc", "g++", True),
    ClangCompiler("clang", "clang", False),
    ClangCompiler("clang", "clang++", True),
    VsCompiler("vs_cl64", "vcvars64.bat", CLCompiler("cl", "cl.exe")),
    VsCompiler("vs_cl32", "vcvars32.bat", CLCompiler("cl", "cl.exe")),
    # VsCompiler("vs_clang64", "vcvars64.bat", ClangCompiler("clang", "clang.exe", False, has_m32=False)),
    # VsCompiler("vs_clang64", "vcvars64.bat", ClangCompiler("clang", "clang++.exe", True, has_m32=False)),
    # VsCompiler("vs_clang32", "vcvars32.bat", ClangCompiler("clang", "clang.exe", False, has_m32=False)),
    # VsCompiler("vs_clang32", "vcvars32.bat", ClangCompiler("clang", "clang++.exe", True, has_m32=False)),
    # EmscriptenCompiler("emcc", emcc", False),
    # EmscriptenCompiler("emcc", emcc++", True),
]

ichain = itertools.chain.from_iterable

for sdk in argv.wasi_sdk:
    path = os.path.join(sdk, "bin", "clang")
    cpp_path = os.path.join(sdk, "bin", "clang++")
    sysroot = os.path.join(sdk, "share", "wasi-sysroot")
    all_compilers += [
        WasiCompiler("wasi_clang", path, False, sysroot),
        WasiCompiler("wasi_clang", cpp_path, True, sysroot),
    ]

for desc in argv.additional_compiler:
    name = re.sub(r"[^A-Za-z0-9\-]", "", desc)
    if "clang" in desc:
        cpp = re.sub(r"clang", "clang++", desc, count=1)
        all_compilers += [
            ClangCompiler(name, desc, False),
            ClangCompiler(name, cpp, True),
        ]
    elif "gcc" in desc:
        cpp = re.sub(r"gcc", "g++", desc, count=1)
        all_compilers += [
            GCCCompiler(name, desc, False),
            GCCCompiler(name, cpp, True),
        ]
    elif "tcc" in desc:
        all_compilers += [
            TCCCompiler(name, desc),
        ]
    else:
        raise RuntimeError(f"Could not parse compiler for {desc}")

if argv.remove_compiler:
    all_compilers = [c for c in all_compilers if c.name not in argv.remove_compiler]

if argv.compiler:
    compilers = set(argv.compiler)
    all_compilers = [c for c in all_compilers if c.name in compilers]

def gather(aws):
    return asyncio.gather(*aws)

async def check_compiler(compiler):
    if await compiler.check_version():
        return [compiler]
    else:
        return []

async def find_compilers():
    return list(ichain(await gather(check_compiler(c) for c in all_compilers)))

class Target:
    def __init__(self, name, suffix, compiler, config):
        self.name = name
        self.suffix = suffix
        self.compiler = compiler
        self.config = config
        self.skipped = False
        self.compiled = False
        self.ran = False
        self.ok = True
        self.log = []

async def compile_target(t):
    arch_test = t.config.get("arch_test", False)
    if config_fmt_arch(t.config) not in t.compiler.compile_archs and not arch_test:
        t.skipped = True
        return

    path = os.path.dirname(t.config["output"])
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)
        log_mkdir(path)

    ok, out, err, cmdline, time = await t.compiler.compile(t.config)

    t.log.append("$ " + cmdline)
    t.log.append(out)
    t.log.append(err)

    if argv.verbose:
        log("\n".join(["$ " + cmdline, out, err]))

    if ok:
        t.compiled = True
    else:
        t.ok = False

    head = f"Compile {t.name}"
    tail = f"[{time:.1f}s OK]" if ok else ("[WARN]" if arch_test else "[FAIL]")
    log_comment(f"{tail} {head}",
        fail=not ok and not arch_test,
        warn=not ok and arch_test)
    return t

class RunOpts(NamedTuple):
    rerunnable: bool = False
    info: str = ""

async def run_target(t, args, opts=RunOpts()):
    if not t.compiled: return

    arch_test = t.config.get("arch_test", False)
    if config_fmt_arch(t.config) not in t.compiler.run_archs and not arch_test:
        return

    args = args[:]
    if t.config.get("dedicated-allocs", False):
        args += ["--dedicated-allocs"]
    args = [a.replace("%TARGET_SUFFIX%", t.suffix) for a in args]

    cwd = t.config.get("cwd")

    if opts.rerunnable and not t.ok:
        return

    if t.config.get("arch") == "wasm32":
        wasm_args = [argv.wasm_runtime, "run", "--dir", ".", t.config["output"], "--"]
        wasm_args += args
        ok, out, err, cmdline, time = await run_cmd(wasm_args, cwd=cwd)
    else:
        ok, out, err, cmdline, time = await run_cmd(t.config["output"], args, cwd=cwd)


    if opts.rerunnable:
        if not t.ok: return

        t.log.clear()
        t.ran = False

    t.log.append("$ " + cmdline)
    t.log.append(out)
    t.log.append(err)

    if argv.verbose:
        log("\n".join(["$ " + cmdline, out, err]))

    if ok:
        t.ran = True
    else:
        t.ok = False

    head = f"Run {t.name}"
    if opts.info:
        head = f"{head} {opts.info}"

    tail = f"[{time:.1f}s OK]" if ok else ("[WARN]" if arch_test else "[FAIL]")
    log_comment(f"{tail} {head}",
        fail=not ok and not arch_test,
        warn=not ok and arch_test)
    return t

async def compile_and_run_target(t, args):
    await compile_target(t)
    if args is not None:
        await run_target(t, args)
    return t

async def run_viewer_target(target):
    log_comment(f"-- Running viewer with {target.name} --")

    for root, _, files in os.walk("data"):
        for file in files:
            if "_fail_" in file: continue
            if "\U0001F602" in file: continue
            path = os.path.join(root, file)
            if "fuzz" in path: continue
            if path.endswith(".fbx") or path.endswith(".obj"):
                target.log.clear()
                target.ran = False
                await run_target(target, [path])
                if not target.ran:
                    return

def copy_file(src, dst):
    shutil.copy(src, dst)
    if sys.platform == "win32":
        log_cmd(f"copy {src} {dst}")
    else:
        log_cmd(f"cp {src} {dst}")

exit_code = 0

def decorate_arch(compiler, arch):
    if arch not in compiler.compile_archs:
        return arch + " (FAIL)"
    if arch not in compiler.run_archs:
        return arch + " (compile only)"
    return arch

tests = set(argv.tests)
implicit_tests = False
if not tests:
    tests = ["tests", "cpp", "stack", "unit", "picort", "viewer", "domfuzz", "objfuzz", "readme", "threadcheck", "hashes"]
    implicit_tests = True

async def main():
    global exit_code

    log_comment(f"-- Running CI tests, using {num_threads} threads --")
    log_comment("-- Searching for compilers --")
    compilers = await find_compilers()

    all_configs = {
        "optimize": {
            "debug": { "optimize": False, "regression": True },
            "release": { "optimize": True },
        },
        "arch": {
            "x86": { "arch": "x86" },
            "x64": { "arch": "x64" },
            "arm32": { "arch": "arm32" },
            "arm64": { "arch": "arm64" },
            "wasm32": { "arch": "wasm32" },
        },
        "sanitize": {
            "": { },
            "sanitize": { "san": True, "dedicated-allocs": True },
        },
    }

    for arch in argv.remove_arch:
        del all_configs["arch"][arch]

    arch_configs = {
        "arch": all_configs["arch"],
    }

    arch_test_configs = {
        "arch": all_configs["arch"],
        "sanitize": all_configs["sanitize"],
    }

    build_path = "build"
    if not os.path.exists(build_path):
        os.makedirs(build_path, exist_ok=True)
        log_mkdir(build_path)

    def compile_permutations(prefix, config, config_options, run_args):
        opt_combos = [[(name, opt) for opt in opts] for name, opts in config_options.items()]
        opt_combos = list(itertools.product(*opt_combos))

        is_cpp = config.get("cpp", False)
        is_c = not is_cpp

        for compiler in compilers:
            archs = set(supported_archs(compiler))

            if is_c and not compiler.has_c: continue
            if is_cpp and not compiler.has_cpp: continue

            for opts in opt_combos:
                optstring = "_".join(opt for _,opt in opts if opt)

                suffix = f"{compiler.name}_{optstring}"
                name = f"{prefix}_{suffix}"

                path = os.path.join(build_path, name)

                conf = copy.deepcopy(config)
                conf["output"] = os.path.join(path, config.get("output", "a.exe"))

                temp_dir = config.get("temp_dir", "")
                if temp_dir:
                    temp_dir = os.path.join(path, temp_dir)
                    conf["temp_dir"] = temp_dir
                    os.makedirs(temp_dir, exist_ok=True)

                if "defines" not in conf:
                    conf["defines"] = { }

                if not conf.get("ignore_user_defines", False):
                    for k in argv.define:
                        v = "1"
                        if "=" in k:
                            k, v = k.split("=", 1)
                        conf["defines"][k] = v

                for opt_name, opt in opts:
                    conf.update(config_options[opt_name][opt])

                overrides = conf.get("overrides")
                if overrides:
                    overrides(conf, compiler)

                if conf.get("skip", False):
                    continue

                if config_fmt_arch(conf) not in archs:
                    continue
                
                target = Target(name, suffix, compiler, conf)
                yield compile_and_run_target(target, run_args)

    exe_suffix = ""
    obj_suffix = ".o"
    if sys.platform == "win32":
        exe_suffix = ".exe"
        obj_suffix = ".obj"

    ctest_tasks = []

    ctest_config = {
        "sources": ["misc/compiler_test.c"],
        "output": "ctest" + exe_suffix,
        "arch_test": True,
        "ignore_user_defines": True,
    }
    ctest_tasks += compile_permutations("ctest", ctest_config, arch_test_configs, ["1.5"])

    cpptest_config = {
        "sources": ["misc/compiler_test.cpp"],
        "output": "cpptest" + exe_suffix,
        "cpp": True,
        "arch_test": True,
        "ignore_user_defines": True,
    }
    ctest_tasks += compile_permutations("cpptest", cpptest_config, arch_test_configs, ["1.5"])

    bad_compiles = 0
    bad_runs = 0

    compiler_test_tasks = await gather(ctest_tasks)
    for target in compiler_test_tasks:
        arch = config_fmt_arch(target.config)
        if target.compiled:
            target.compiler.compile_archs.add(arch)
        else:
            bad_compiles += 1
        if target.ran and "sin(1.50) = 1.00" in target.log:
            target.compiler.run_archs.add(arch)
        elif target.compiled:
            bad_runs += 1

    log_comment("-- Detected the following compilers --")

    removed_archs = set(argv.remove_arch)
    removed_archs.update(f"{arch}-san" for arch in argv.remove_arch)

    for compiler in compilers:
        archs = ", ".join(decorate_arch(compiler, arch) for arch in supported_archs(compiler) if arch not in removed_archs)
        log_comment(f"  {compiler.exe}: {compiler.arch} {compiler.version} [{archs}]")

    num_fail = 0
    all_targets = []

    if "tests" in tests:
        log_comment("-- Compiling and running tests --")

        target_tasks = []

        def platform_overrides(config, compiler):
            use_threads = True
            if config["arch"] in ["wasm32"]:
                use_threads = False
            if compiler.name in ["tcc"]:
                use_threads = False

            if use_threads:
                config["threads"] = True
                config["defines"]["UFBXT_THREADS"] = ""

        runner_config = {
            "sources": ["test/runner.c", "ufbx.c"],
            "output": "runner" + exe_suffix,
            "defines": { },
            "overrides": platform_overrides,
        }
        target_tasks += compile_permutations("runner", runner_config, all_configs, ["-d", "data"])

        float_config = {
            "sources": ["test/runner.c", "ufbx.c"],
            "output": "runner_float" + exe_suffix,
            "defines": {
                "UFBX_REAL_IS_FLOAT": 1,
            }
        }
        target_tasks += compile_permutations("runner_float", float_config, arch_configs, ["-d", "data"])

        warnings_config = {
            "sources": ["misc/test_build.c"],
            "output": "c" + exe_suffix,
            "warnings": True,
            "overrides": platform_overrides,
        }
        target_tasks += compile_permutations("warnings", warnings_config, arch_configs, [])

        cpp_config = {
            "sources": ["misc/test_build.cpp"],
            "output": "cpp" + exe_suffix,
            "cpp": True,
            "warnings": True,
            "overrides": platform_overrides,
        }
        target_tasks += compile_permutations("cpp", cpp_config, arch_configs, [])

        cpp_no_dev_config = {
            "sources": ["misc/test_build.cpp"],
            "output": "cpp" + exe_suffix,
            "cpp": True,
            "warnings": True,
            "optimize": True,
            "dev": False,
            "overrides": platform_overrides,
        }
        target_tasks += compile_permutations("cpp_no_dev", cpp_no_dev_config, arch_configs, [])

        targets = await gather(target_tasks)
        all_targets += targets

    if "freestanding" in tests:
        log_comment("-- Compiling and running freestanding tests --")

        target_tasks = []

        freestanding_config = {
            "sources": ["extra/ufbx_math.c", "extra/ufbx_libc.c", "misc/ufbx_malloc.c", "misc/ufbx_libc_os.c", "test/runner.c", "ufbx.c"],
            "output": "freestanding_runner" + exe_suffix,
            "defines": {
                "UFBX_NO_LIBC": "",
            },
        }
        target_tasks += compile_permutations("freestanding_runner", freestanding_config, arch_configs, ["-d", "data"])

        targets = await gather(target_tasks)
        all_targets += targets

    if "cpp" in tests:
        log_comment("-- Compiling and running C++ tests --")

        target_tasks = []

        cpp_configs = all_configs.copy()
        if not argv.heavy:
            del cpp_configs["sanitize"]
        cpp_configs.update({
            "cpp": {
                "cpp11": { "std": "c++11" },
                "cpp14": { "std": "c++14" },
                "cpp17": { "std": "c++17" },
                "cpp20": { "std": "c++20" },
            },
        })

        # MSVC-based C++ standard libraries don't support C++11
        if sys.platform == "win32":
            del cpp_configs["cpp"]["cpp11"]

        runner_config = {
            "sources": ["test/cpp/cpp_runner.cpp", "ufbx.c"],
            "output": "cpp_runner" + exe_suffix,
            "cpp": True,
        }
        target_tasks += compile_permutations("cpp_runner", runner_config, cpp_configs, ["-d", "data"])

        targets = await gather(target_tasks)
        all_targets += targets

    if "stack" in tests:
        log_comment("-- Compiling and running stack limited tests --")

        target_tasks = []

        def debug_overrides(config, compiler):
            stack_limit = 256*1024
            if sys.platform == "win32" and compiler.name in ["gcc"]:
                # GCC can't handle CreateThread on CI..
                config["skip"] = True
            elif sys.platform == "win32" and compiler.name in ["clang"]:
                # TODO: Check what causes this stack usage
                stack_limit = 256*1024
            config["defines"]["UFBXT_STACK_LIMIT"] = stack_limit

        debug_stack_config = {
            "sources": ["test/runner.c", "ufbx.c"],
            "output": "runner" + exe_suffix,
            "optimize": False,
            "threads": True,
            "stack_protector": True,
            "defines": { },
            "overrides": debug_overrides,
        }

        target_tasks += compile_permutations("runner_debug_stack", debug_stack_config, arch_configs, ["-d", "data"])

        def release_overrides(config, compiler):
            stack_limit = 128*1024
            if sys.platform == "win32" and compiler.name in ["gcc"]:
                # GCC can't handle CreateThread on CI..
                config["skip"] = True
            elif sys.platform == "win32" and compiler.name in ["clang", "vs_cl64"]:
                # TODO: Check what causes this stack usage
                stack_limit = 128*1024
            elif config["arch"] == "arm64":
                # Fails with 'Failed to run thread with stack size of 65536 bytes'
                # with 64kiB stack..
                stack_limit = 128*1024
            config["defines"]["UFBXT_STACK_LIMIT"] = stack_limit

        release_stack_config = {
            "sources": ["test/runner.c", "ufbx.c"],
            "output": "runner" + exe_suffix,
            "optimize": True,
            "threads": True,
            "stack_protector": True,
            "defines": { },
            "overrides": release_overrides,
        }

        target_tasks += compile_permutations("runner_release_stack", release_stack_config, arch_configs, ["-d", "data"])

        targets = await gather(target_tasks)
        all_targets += targets

    if "unit" in tests:
        log_comment("-- Compiling and running unit tests --")

        target_tasks = []

        runner_config = {
            "sources": ["test/unit_tests.c"],
            "output": "unit_tests" + exe_suffix,
            "defines": { },
        }
        target_tasks += compile_permutations("unit_tests", runner_config, all_configs, [])

        runner_config = {
            "sources": ["test/extra/test_math.c"],
            "output": "math_tests" + exe_suffix,
            "optimize": True,
            "defines": { },
        }
        target_tasks += compile_permutations("math_tests", runner_config, arch_configs, [])

        targets = await gather(target_tasks)
        all_targets += targets

    if "features" in tests:
        log_comment("-- Compiling and running partial features --")

        feature_defines = [
            "UFBX_NO_SUBDIVISION",
            "UFBX_NO_TESSELLATION",
            "UFBX_NO_GEOMETRY_CACHE",
            "UFBX_NO_SCENE_EVALUATION",
            "UFBX_NO_SKINNING_EVALUATION",
            "UFBX_NO_ANIMATION_BAKING",
            "UFBX_NO_FORMAT_OBJ",
            "UFBX_NO_INDEX_GENERATION",
            "UFBX_NO_TRIANGULATION",
            "UFBX_NO_ERROR_STACK",
            "UFBX_REAL_IS_FLOAT",
            "UFBX_NO_STDIO",
        ]

        target_tasks = []

        for bits in range(0, 1 << len(feature_defines)):
            defines = { name: 1 for ix, name in enumerate(feature_defines) if (1 << ix) & bits }

            # Only consider up to two positive or negative features
            if not argv.heavy and not (len(defines) <= 2 or len(defines) >= len(feature_defines) - 2):
                continue

            feature_config = {
                "sources": ["ufbx.c", "misc/minimal_main.c"],
                "output": f"features_{bits}" + exe_suffix,
                "temp_dir": os.path.join("temp", f"features_{bits}"),
                "warnings": True,
                "defines": defines,
            }
            target_tasks += compile_permutations("features", feature_config, arch_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

    if "picort" in tests:
        log_comment("-- Compiling and running picort --")
    
        target_tasks = []

        picort_configs = {
            "arch": arch_configs["arch"],
        }

        picort_config = {
            "sources": [
                "ufbx.c",
                "examples/picort/picort.cpp",
                "examples/picort/picort_bvh.cpp",
                "examples/picort/picort_gui.cpp",
                "examples/picort/picort_opts.cpp",
                "examples/picort/picort_png.cpp",
            ],
            "output": "picort" + exe_suffix,
            "cpp": True,
            "optimize": True,
            "std": "c++14",
            "threads": True,
        }
        target_tasks += compile_permutations("picort", picort_config, picort_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

        def target_score(target, want_sse):
            compiler = target.compiler
            config = target.config
            if not target.compiled:
                return (0, 0)
            score = 1
            if config.get("sse", False) == want_sse:
                score += 100
            if config["arch"] == "x64":
                score += 10
            if "vs_cl64" in compiler.name:
                score += 20
            if "clang" in compiler.name:
                score += 10
            if "msvc" in compiler.name:
                score += 5
            version = re.search(r"\d+", compiler.version)
            version = int(version.group(0)) if version else 0
            return (score, version)

        image_path = os.path.join("build", "images")
        if not os.path.exists(image_path):
            os.makedirs(image_path, exist_ok=True)
            log_mkdir(image_path)

        target = max(targets, key=lambda t: target_score(t, True))
        
        if target.compiled:
            log_comment(f"-- Rendering scenes with {target.name} --")

            scenes = [
                "data/picort/barbarian.picort.txt",
                "data/picort/slime-binary.picort.txt",
                "data/picort/material-chart.picort.txt",
                "data/picort/blender-material-chart.picort.txt",
                "data/picort/maze.picort.txt",
            ]

            for scene in scenes:
                target.log.clear()
                target.ran = False
                args = [scene]
                if argv.strict:
                    args += ["--error-threshold", "0.000001"]
                await run_target(target, args)
                if not target.ran:
                    break
                for line in target.log[1].splitlines(keepends=False):
                    log_comment(line)

    if "viewer" in tests:
        log_comment("-- Compiling and running viewer --")
    
        target_tasks = []

        viewer_configs = {
            "arch": arch_configs["arch"],
            "sanitize": {
                "": { },
                "sanitize": { "san": True },
            }
        }

        viewer_config = {
            "sources": ["ufbx.c", "examples/viewer/viewer.c", "examples/viewer/external.c"],
            "output": "viewer" + exe_suffix,
            "optimize": True,
            "defines": {
                "TEST_VIEWER": "",
            },
        }
        target_tasks += compile_permutations("viewer", viewer_config, viewer_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

        running_viewers = []
        for target in targets:
            if target.compiled:
                running_viewers.append(run_viewer_target(target))
        await gather(running_viewers)

    if "domfuzz" in tests:
        log_comment("-- Compiling and running domfuzz --")
    
        target_tasks = []

        domfuzz_configs = {
            "arch": arch_configs["arch"],
        }

        domfuzz_config = {
            "sources": ["ufbx.c", "test/domfuzz/fbxdom.cpp", "test/domfuzz/domfuzz_main.cpp"],
            "output": "domfuzz" + exe_suffix,
            "cpp": True,
            "optimize": True,
            "std": "c++14",
        }
        target_tasks += compile_permutations("domfuzz", domfuzz_config, domfuzz_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

        def target_score(target):
            compiler = target.compiler
            config = target.config
            if not target.compiled:
                return (0, 0)
            score = 1
            if config["arch"] == "x64":
                score += 10
            if "clang" in compiler.name:
                score += 10
            if "msvc" in compiler.name:
                score += 5
            version = re.search(r"\d+", compiler.version)
            version = int(version.group(0)) if version else 0
            return (score, version)

        target = max(targets, key=target_score)

        if target.compiled:
            log_comment(f"-- Running domfuzz with {target.name} --")

            too_heavy_files = [
                "blender_293_barbarian_7400_binary",
                "maya_slime_7500_binary",
                "maya_character_6100_binary",
                "maya_character_7500_binary",
                "maya_human_ik_6100_binary",
                "maya_human_ik_7400_binary",
                "max2009_blob_6100_binary",
                "synthetic_unicode_7500_binary",
                "maya_lod_group_6100_binary",
                "maya_dq_weights_7500_binary",
                "maya_triangulate_triangulated_7500_binary",
                "maya_kenney_character_7700_binary",
                "maya_node_attribute_zoo_6100_binary",
                "max_physical_material_textures_6100_binary",
            ]

            run_tasks = []
            for root, _, files in os.walk("data"):
                for file in files:
                    if "_ascii" in file: continue
                    if any(f in file for f in too_heavy_files) and not argv.heavy: continue
                    path = os.path.join(root, file)
                    if "fuzz" in path: continue
                    if path.endswith(".fbx"):
                        target.log.clear()
                        target.ran = False
                        run_tasks.append(run_target(target, [path],
                            RunOpts(
                                rerunnable=True,
                                info=path,
                            )
                        ))
            
            targets = await gather(run_tasks)

    if "objfuzz" in tests:
        log_comment("-- Compiling and running objfuzz --")
    
        target_tasks = []

        objfuzz_configs = {
            "arch": arch_configs["arch"],
        }

        objfuzz_config = {
            "sources": ["ufbx.c", "test/objfuzz.cpp"],
            "output": "objfuzz" + exe_suffix,
            "cpp": True,
            "optimize": True,
            "std": "c++14",
        }
        target_tasks += compile_permutations("objfuzz", objfuzz_config, objfuzz_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

        def target_score(target):
            compiler = target.compiler
            config = target.config
            if not target.compiled:
                return (0, 0)
            score = 1
            if config["arch"] == "x64":
                score += 10
            if "clang" in compiler.name:
                score += 10
            if "msvc" in compiler.name:
                score += 5
            version = re.search(r"\d+", compiler.version)
            version = int(version.group(0)) if version else 0
            return (score, version)

        target = max(targets, key=target_score)

        if target.compiled:
            log_comment(f"-- Running objfuzz with {target.name} --")

            too_heavy_files = [
                "synthetic_color_suzanne_0_obj.obj",
                "synthetic_color_suzanne_1_obj.obj",
                "zbrush_polygroup_mess_0_obj.obj",
            ]

            run_tasks = []
            for root, _, files in os.walk("data"):
                for file in files:
                    path = os.path.join(root, file)
                    if any(f in file for f in too_heavy_files) and not argv.heavy: continue
                    if re.match(r"^.*_\d+_obj.obj$", file):
                        target.log.clear()
                        target.ran = False
                        run_tasks.append(run_target(target, [path],
                            RunOpts(
                                rerunnable=True,
                                info=path,
                            )
                        ))
                    elif re.match(r"^.*_\d+_mtl.mtl$", file):
                        target.log.clear()
                        target.ran = False
                        run_tasks.append(run_target(target, ["--mtl", path],
                            RunOpts(
                                rerunnable=True,
                                info=path,
                            )
                        ))
            
            targets = await gather(run_tasks)

    if "readme" in tests:
        log_comment("-- Compiling and running README.md --")

        prologue = """
            #include <stdio.h>
            #include <stdlib.h>
            #include "../ufbx.h"

            void push_vertex(const ufbx_vec3 *position, const ufbx_vec3 *normal) { }
            void push_pose(const ufbx_matrix *matrix) { }

            int main(int argc, char **argv) {
        """

        epilogue = """
            return 0;
            }
        """

        readme_dst = os.path.join(build_path, "readme.c")
        with open(readme_dst, "wt") as outf:
            for line in prologue.strip().splitlines():
                print(line.strip(), file=outf)

            in_c = False
            with open("README.md", "rt") as inf:
                for line in inf:
                    if line.strip() == "```c":
                        in_c = True
                    elif line.strip() == "```":
                        in_c = False
                    elif in_c:
                        print(line.rstrip(), file=outf)

            for line in epilogue.strip().splitlines():
                print(line.strip(), file=outf)

        readme_cpp_dst = os.path.join(build_path, "readme.cpp")
        shutil.copyfile(readme_dst, readme_cpp_dst)

        copy_file(
            os.path.join("data" ,"blender_279_default_7400_binary.fbx"),
            os.path.join(build_path, "thing.fbx"))

        target_tasks = []

        readme_config = {
            "sources": ["build/readme.c", "ufbx.c"],
            "output": "readme" + exe_suffix,
            "cwd": "build",
        }
        target_tasks += compile_permutations("readme", readme_config, arch_configs, [])

        readme_cpp_config = {
            "sources": ["build/readme.cpp", "ufbx.c"],
            "output": "readme_cpp" + exe_suffix,
            "cpp": True,
            "cwd": "build",
        }
        target_tasks += compile_permutations("readme_cpp", readme_cpp_config, arch_configs, [])

        targets = await gather(target_tasks)
        all_targets += targets

    if "threadcheck" in tests:
        log_comment("-- Compiling and running threadcheck --")
    
        target_tasks = []

        threadcheck_config = {
            "sources": ["ufbx.c", "test/threadcheck.cpp"],
            "output": "threadcheck" + exe_suffix,
            "cpp": True,
            "optimize": False,
            "std": "c++14",
            "threads": True,
        }
        target_tasks += compile_permutations("threadcheck", threadcheck_config, arch_configs, None)

        targets = await gather(target_tasks)
        all_targets += targets

        def target_score(target):
            compiler = target.compiler
            config = target.config
            if not target.compiled:
                return (0, 0)
            score = 1
            if config["arch"] == "x64":
                score += 10
            if "clang" in compiler.name:
                score += 10
            if "msvc" in compiler.name:
                score += 5
            version = re.search(r"\d+", compiler.version)
            version = int(version.group(0)) if version else 0
            return (score, version)

        best_target = max(targets, key=target_score)
        if best_target.compiled:
            log_comment(f"-- Running {best_target.name} --")

            best_target.log.clear()
            best_target.ran = False
            await run_target(best_target, ["data/maya_cube_7500_binary.fbx", "2"])
            for line in best_target.log[1].splitlines(keepends=False):
                log_comment(line)

    if "hashes" in tests:

        hash_file = argv.hash_file
        if not hash_file:
            hash_file = os.path.join(build_path, "hashes.txt")
        if hash_file and os.path.exists(hash_file):
            log_comment("-- Compiling and running hash_scene --")
            target_tasks = []

            hash_scene_configs = {
                "arch": all_configs["arch"],
                "optimize": all_configs["optimize"],
            }

            hash_scene_config = {
                "sources": ["test/hash_scene.c", "extra/ufbx_math.c"],
                "output": "hash_scene" + exe_suffix,
                "ieee754": True,
                "defines": { },
            }

            if argv.hash_threads:
                hash_scene_config["threads"] = True
                hash_scene_config["defines"]["USE_THREADS"] = ""
                hash_scene_config["defines"]["UFBX_EXTENSIVE_THREADING"] = ""

            dump_path = os.path.join(build_path, "hashdumps")
            if not os.path.exists(dump_path):
                os.makedirs(dump_path, exist_ok=True)
                log_mkdir(dump_path)
            else:
                for dump_file in os.listdir(dump_path):
                    assert dump_file.endswith(".txt")
                    os.remove(os.path.join(dump_path, dump_file))

            if argv.runner:
                dump_file = os.path.join(dump_path, f"{argv.runner}_%TARGET_SUFFIX%.txt")
            else:
                dump_file = os.path.join(dump_path, "%TARGET_SUFFIX%.txt")

            args = ["--check", hash_file, "--dump", dump_file, "--max-dump-errors", "3"]
            target_tasks += compile_permutations("hash_scene", hash_scene_config, hash_scene_configs, args)

            targets = await gather(target_tasks)
            all_targets += targets
        else:
            print("No hash file found, skipping hashes", file=sys.stderr)
            if not implicit_tests:
                exit_code = 2


    for target in all_targets:
        if target.ok: continue
        print()
        log(f"-- FAIL: {target.name}", style=STYLE_FAIL)
        print("\n".join(target.log))
        num_fail += 1

    print()
    severity = "ERROR" if argv.fail_on_pre_test else "WARNING"
    if bad_compiles > 0:
        print(f"{severity}: {bad_compiles} pre-test configurations failed to compile")
        if argv.fail_on_pre_test:
            exit_code = 2
    if bad_runs > 0:
        print(f"{severity}: {bad_runs} pre-test configurations failed to run")
        if argv.fail_on_pre_test:
            exit_code = 2
    print(f"{len(all_targets) - num_fail}/{len(all_targets)} targets succeeded")
    if num_fail > 0:
        exit_code = 1

if __name__ == "__main__":
    asyncio.run(main())
    sys.exit(exit_code)
