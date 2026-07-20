import parsette
import string
from typing import List, Optional, NamedTuple, Union
import json
import argparse
import os
import re

lexer = parsette.Lexer()

lexer.ignore_whitespace()

TEnd = parsette.End
TIdent = lexer.rule("identifier", r"[A-Za-z_][A-Za-z0-9_]*", prefix=string.ascii_letters+"_")
TNumber = lexer.rule("number", r"(0[Xx][0-9A-Fa-f]+)|([0-9]+)", prefix=string.digits)
TComment = lexer.rule("comment", r"//[^\r\n]*", prefix="/")
TPreproc = lexer.rule("preproc", r"#[^\n\\]*(\\\r?\n[^\n\\]*?)*\n", prefix="#")
TString = lexer.rule("string", r"\"[^\"]*\"", prefix="\"")
lexer.literals(*"const typedef struct union enum extern ufbx_abi ufbx_abi_data ufbx_abi_data_def ufbx_inline ufbx_nullable ufbx_unsafe UFBX_LIST_TYPE UFBX_ENUM_REPR UFBX_FLAG_REPR UFBX_ENUM_FORCE_WIDTH UFBX_FLAG_FORCE_WIDTH UFBX_ENUM_TYPE".split())
lexer.literals(*",.*[]{}()<>=-?:;")
lexer.ignore("disable", re.compile(r"//\s*bindgen-disable.*?//\s*bindgen-enable", flags=re.DOTALL))

Token = parsette.Token
Ast = parsette.Ast

class AType(Ast):
    pass

class AName(Ast):
    pass

class ATop(Ast):
    pass

class AStructDecl(Ast):
    pass

class AEnumDecl(Ast):
    pass

class ADecl(Ast):
    type: AType
    names: List[AName]
    end_line: Optional[int] = None

class ANamePointer(AName):
    inner: AName

class ANameArray(AName):
    inner: AName
    length: Optional[Token]

class ANameIdent(AName):
    ident: Token

class ANameFunction(AName):
    inner: AName
    args: List[ADecl]

class ANameAnonymous(AName):
    pass

class ATypeConst(AType):
    inner: AType

class ATypeSpec(AType):
    inner: AType
    spec: Token

class ATypeIdent(AType):
    name: Token

class ATypeStruct(AType):
    kind: Token
    name: Optional[Token]
    decls: Optional[List[AStructDecl]]

class ATypeEnum(AType):
    kind: Token
    name: Optional[Token]
    decls: Optional[List[AEnumDecl]]

class AStructComment(AStructDecl):
    comments: List[Token]

class AStructField(AStructDecl):
    decl: ADecl

class AEnumComment(AEnumDecl):
    comments: List[Token]

class AEnumValue(AEnumDecl):
    name: Token
    value: Optional[Token]

class ATopPreproc(ATop):
    preproc: Token

class ATopComment(ATop):
    comments: List[Token]

class ATopDecl(ATop):
    decl: ADecl

class ATopExtern(ATop):
    decl: ADecl

class ATopTypedef(ATop):
    decl: ADecl

class ATopFile(ATop):
    tops: List[ATop]

class ATopList(ATop):
    name: Token
    type: ADecl

class ATopEnumType(ATop):
    enum_type: Token
    prefix: Token
    last_value: Token

class Parser(parsette.Parser):
    def __init__(self, source, filename=""):
        super().__init__(lexer, source, filename)

    def finish_comment(self, comment_type, first):
        comments = [first]
        line = first.location.line + 1
        while self.peek(TComment) and self.token.location.line == line:
            comments.append(self.scan())
            line += 1
        return comment_type(comments)

    def accept_impl(self) -> bool:
        if self.token.rule != TIdent: return False
        text = self.token.text()
        if not text.startswith("UFBX_"): return False
        if not text.endswith("_IMPL"): return False
        self.scan()
        return True

    def finish_struct(self, kind) -> ATypeStruct:
        kn = kind.text()
        name = self.accept(TIdent)
        if self.accept("{"):
            fields = []
            loc = name if name else kind
            with self.hint(loc, f"{kn} {name.text()}" if name else f"anonymous {kn}"):
                while not self.accept("}"):
                    if self.accept(TComment):
                        fields.append(self.finish_comment(AStructComment, self.prev_token))
                    elif self.accept_impl():
                        self.require("(", "for macro parameters")
                        self.finish_macro_params()
                    else:
                        decl = self.parse_decl(f"{kn} field")
                        field = AStructField(decl)
                        fields.append(field)
                        self.require(";", f"after {kn} field")
        else:
            fields = None
        return ATypeStruct(kind, name, fields)
    
    def parse_enum_decl(self) -> AEnumDecl:
        if self.accept(TComment):
            return self.finish_comment(AEnumComment, self.prev_token)
        else:
            name = self.require(TIdent, "enum value name")
            value = None
            if self.accept("="):
                value = self.require([TIdent, TNumber], f"'{name.text()}' value")
            return AEnumValue(name, value)

    def finish_enum(self, kind) -> ATypeStruct:
        kn = kind.text()
        name = self.accept(TIdent)
        self.require(["UFBX_ENUM_REPR", "UFBX_FLAG_REPR"], "enum repr macro")
        if self.accept("{"):
            decls = []
            loc = name if name else kind
            has_force_width = False
            with self.hint(loc, f"{kn} {name.text()}" if name else f"anonymous {kn}"):
                while not self.accept("}"):
                    if self.accept(","):
                        continue
                    if self.accept(["UFBX_ENUM_FORCE_WIDTH", "UFBX_FLAG_FORCE_WIDTH"]):
                        self.require("(", "for FORCE_WIDTH macro parameters")
                        self.require(TIdent, "for FORCE_WIDTH macro name")
                        self.require(")", "for FORCE_WIDTH macro parameters")
                        has_force_width = True
                        continue
                    decls.append(self.parse_enum_decl())
                if not has_force_width:
                    self.fail_at(self.prev_token, "enum missing FORCE_WIDTH macro")
        else:
            decls = None
        return ATypeEnum(kind, name, decls)

    def parse_type(self) -> AType:
        token = self.token
        if self.accept("const"):
            inner = self.parse_type()
            return ATypeConst(inner)
        elif self.accept(["ufbx_nullable", "ufbx_abi", "ufbx_abi_data", "ufbx_unsafe", "ufbx_inline"]):
            inner = self.parse_type()
            return ATypeSpec(inner, token)
        elif self.accept(["struct", "union"]):
            return self.finish_struct(self.prev_token)
        elif self.accept("enum"):
            return self.finish_enum(self.prev_token)
        elif self.accept(TIdent):
            return ATypeIdent(self.prev_token)
        else:
            self.fail_got("expected a type")

    def parse_name_non_array(self, ctx, allow_anonymous=False) -> AName:
        if self.accept("*"):
            inner = self.parse_name_non_array(ctx, allow_anonymous)
            return ANamePointer(inner)
        if allow_anonymous and not self.peek(TIdent):
            return ANameAnonymous()
        else:
            name = self.require(TIdent, f"for {ctx} name")
            return ANameIdent(name)

    def parse_name(self, ctx, allow_anonymous=False) -> AName:
        ast = self.parse_name_non_array(ctx, allow_anonymous)

        while True:
            if self.accept("["):
                length = self.accept([TIdent, TNumber])
                self.require("]", f"for opening [")
                ast = ANameArray(ast, length)
            elif self.accept("("):
                args = []
                while not self.accept(")"):
                    args.append(self.parse_decl("argument", allow_list=False, allow_anonymous=True))
                    self.accept(",")
                ast = ANameFunction(ast, args)
            else:
                break
        return ast

    def parse_decl(self, ctx, allow_anonymous=False, allow_list=True) -> ADecl:
        typ = self.parse_type()
        names = []
        if not self.peek(";"):
            if allow_list:
                for _ in self.sep(","):
                    names.append(self.parse_name(ctx, allow_anonymous))
            else:
                names.append(self.parse_name(ctx, allow_anonymous))
        return ADecl(typ, names)
    
    def finish_top_list(self) -> ATopList:
        self.require("(", "for macro parameters")
        name = self.require(TIdent, "for list type name")
        self.require(",", "for macro parameters")
        decl = self.parse_decl("UFBX_TOP_LIST type", allow_anonymous=True, allow_list=False)
        self.require(")", "for macro parameters")
        return ATopList(name, decl)

    def finish_top_enum_type(self) -> ATopEnumType:
        self.require("(", "for macro parameters")
        enum_name = self.require(TIdent, "for enum type name")
        self.require(",", "for macro parameters")
        prefix = self.require(TIdent, "for enum prefix")
        self.require(",", "for macro parameters")
        last_value = self.require(TIdent, "for enum last value")
        self.require(")", "for macro parameters")
        return ATopEnumType(enum_name, prefix, last_value)

    def finish_macro_params(self):
        while not self.accept(")"):
            if self.accept(TEnd): self.fail("Unclosed macro parameters")
            if self.accept("("):
                self.finish_macro_params()
            else:
                self.scan()

    def parse_top(self) -> List[ATop]:
        if self.accept(TPreproc):
            return [ATopPreproc(self.prev_token)]
        elif self.accept(TComment):
            return [self.finish_comment(ATopComment, self.prev_token)]
        elif self.accept("typedef"):
            decl = self.parse_decl("typedef")
            self.require(";", "after typedef")
            decl.end_line = self.prev_token.location.line
            return [ATopTypedef(decl)]
        elif self.accept("extern"):
            if self.accept(TString):
                self.require("{", "for extern ABI block")
                tops = []
                while not self.accept("}"):
                    tops += self.parse_top()
                return tops
            else:
                decl = self.parse_decl("extern")
                self.require(";", "after extern")
                decl.end_line = self.prev_token.location.line
                return [ATopExtern(decl)]
        elif self.accept("UFBX_LIST_TYPE"):
            tl = self.finish_top_list()
            self.require(";", "after UFBX_LIST_TYPE()")
            return [tl]
        elif self.accept("UFBX_ENUM_TYPE"):
            tl = self.finish_top_enum_type()
            self.require(";", "after UFBX_ENUM_TYPE()")
            return [tl]
        else:
            decl = self.parse_decl("top-level")
            if self.accept("{"):
                level = 1
                while level > 0:
                    if self.accept("{"):
                        level += 1
                    elif self.accept("}"):
                        level -= 1
                    else:
                        self.scan()
                decl.end_line = self.prev_token.location.line
            else:
                self.require(";", "after top-level declaration")
                decl.end_line = self.prev_token.location.line
            return [ATopDecl(decl)]

    def parse_top_file(self) -> ATopFile:
        tops = []
        while not self.accept(parsette.End):
            if self.ignore(TEnd): continue
            tops += self.parse_top()
        return ATopFile(tops)

def fmt_type(type: AType):
    if isinstance(type, ATypeIdent):
        return type.name.text()
    elif isinstance(type, ATypeConst):
        return f"const {fmt_type(type.inner)}"
    elif isinstance(type, ATypeSpec):
        return f"{type.spec.text()} {fmt_type(type.inner)}"

class SMod: pass
class SModConst(SMod): pass
class SModNullable(SMod): pass
class SModInline(SMod): pass
class SModAbi(SMod): pass
class SModAbiData(SMod): pass
class SModUnsafe(SMod): pass
class SModPointer(SMod): pass
class SModArray(SMod):
    def __init__(self, length: Optional[str]):
        self.length = length
class SModFunction(SMod):
    def __init__(self, args: List["SDecl"]):
        self.args = args

class SComment(NamedTuple):
    line_begin: int
    line_end: int
    text: List[str]

class SType(NamedTuple):
    kind: str
    name: Optional[str]
    mods: List[SMod] = []
    body: Union["SStruct", "SEnum", "SEnumType", None] = None

class SName(NamedTuple):
    name: Optional[str]
    type: SType
    value: Optional[str] = None

class SDecl(NamedTuple):
    line_begin: int
    line_end: int
    kind: str
    names: List[SName]
    comment: Optional[SComment] = None
    comment_inline: bool = False
    is_function: bool = False
    define_args: Optional[List[str]] = None
    value: Optional[str] = None

class SDeclGroup(NamedTuple):
    line: int
    decls: List[SDecl]
    comment: Optional[SComment] = None
    comment_inline: bool = False
    is_function: bool = False

SCommentDecl = Union[SComment, SDecl, SDeclGroup]

class SStruct(NamedTuple):
    line: int
    kind: str
    name: Optional[str]
    decls: List[SCommentDecl]
    is_list: bool = False

class SEnum(NamedTuple):
    line: int
    name: Optional[str]
    decls: List[SCommentDecl]

class SEnumType(NamedTuple):
    line: int
    enum_name: str
    enum_prefix: str
    last_value: str

def type_line(typ: AType):
    if isinstance(typ, ATypeIdent):
        return typ.name.location.line
    elif isinstance(typ, ATypeConst):
        return type_line(typ.inner)
    elif isinstance(typ, ATypeStruct):
        return typ.kind.location.line
    elif isinstance(typ, ATypeEnum):
        return typ.kind.location.line
    elif isinstance(typ, ATypeSpec):
        return type_line(typ.inner)
    else:
        raise TypeError(f"Unhandled type {type(typ).__name__}")

spec_to_mod = {
    "ufbx_abi": SModAbi,
    "ufbx_abi_data": SModAbiData,
    "ufbx_nullable": SModNullable,
    "ufbx_inline": SModInline,
    "ufbx_unsafe": SModUnsafe,
}

def to_stype(typ: AType) -> SType:
    if isinstance(typ, ATypeIdent):
        return SType("name", typ.name.text())
    elif isinstance(typ, ATypeConst):
        st = to_stype(typ.inner)
        return st._replace(mods=st.mods + [SModConst()])
    elif isinstance(typ, ATypeSpec):
        st = to_stype(typ.inner)
        spec = typ.spec.text()
        return st._replace(mods=st.mods + [spec_to_mod[spec]()])
    elif isinstance(typ, ATypeStruct):
        body = to_sstruct(typ) if typ.decls is not None else None
        return SType(typ.kind.text(), typ.name.text() if typ.name else None, body=body)
    elif isinstance(typ, ATypeEnum):
        body = to_senum(typ) if typ.decls is not None else None
        return SType("enum", typ.name.text() if typ.name else None, body=body)
    else:
        raise TypeError(f"Unhandled type {type(typ).__name__}")

def name_to_stype(base: SType, name: AName) -> SType:
    if isinstance(name, ANamePointer):
        st = name_to_stype(base, name.inner)
        return st._replace(mods=st.mods + [SModPointer()])
    elif isinstance(name, ANameArray):
        st = name_to_stype(base, name.inner)
        mod = SModArray(name.length.text() if name.length else None)
        return st._replace(mods=st.mods + [mod])
    elif isinstance(name, ANameFunction):
        st = name_to_stype(base, name.inner)
        mod = SModFunction([to_sdecl(a, "argument") for a in name.args])
        return st._replace(mods=st.mods + [mod])
    elif isinstance(name, ANameIdent):
        return base
    elif isinstance(name, ANameAnonymous):
        return base
    else:
        raise TypeError(f"Unhandled type {type(name)}")

def name_str(name: AName):
    if isinstance(name, ANameIdent):
        return name.ident.text()
    elif isinstance(name, ANameAnonymous):
        return None
    elif isinstance(name, ANamePointer):
        return name_str(name.inner)
    elif isinstance(name, ANameArray):
        return name_str(name.inner)
    elif isinstance(name, ANameFunction):
        return name_str(name.inner)
    else:
        raise TypeError(f"Unhandled type {type(name)}")

def to_sdecl(decl: ADecl, kind: str) -> SDecl:
    names = []
    is_function = False
    base_st = to_stype(decl.type)
    for name in decl.names:
        st = name_to_stype(base_st, name)
        if any(isinstance(mod, SModFunction) for mod in st.mods):
            is_function = True
        names.append(SName(name_str(name), st))
    if not decl.names:
        names.append(SName(None, base_st))
    line = type_line(decl.type)
    end_line = decl.end_line
    if end_line is None: end_line = line
    return SDecl(line, end_line, kind, names, is_function=is_function)

Comment = List[str]

def to_scomment(comment: Ast):
    if not comment: return None
    begin = comment.comments[0].location.line
    end = comment.comments[-1].location.line
    text = [c.text()[3:] for c in comment.comments]
    return SComment(begin, end, text)

def to_sstruct(struct: ATypeStruct) -> SStruct:
    decls = []

    for decl in struct.decls:
        if isinstance(decl, AStructComment):
            decls.append(to_scomment(decl))
        elif isinstance(decl, AStructField):
            decls.append(to_sdecl(decl.decl, "field"))

    line = struct.kind.location.line
    name = struct.name.text() if struct.name else None
    kind = struct.kind.text()
    return SStruct(line, kind, name, decls)

def to_senum(enum: ATypeEnum) -> SEnum:
    decls = []
    name = enum.name.text() if enum.name else None

    for decl in enum.decls:
        if isinstance(decl, AEnumComment):
            decls.append(to_scomment(decl))
        elif isinstance(decl, AEnumValue):
            line = decl.name.location.line
            decls.append(SDecl(
                line_begin=line,
                line_end=line,
                kind="enumValue",
                value=decl.value.text() if decl.value else None,
                names=[
                    SName(
                        name=decl.name.text(),
                        type=SType("enum", name),
                        value=decl.value)
                ]))

    line = enum.kind.location.line
    return SEnum(line, name, decls)

def to_sbody(typ: AType):
    if isinstance(typ, ATypeStruct):
        return to_sstruct(typ)
    elif isinstance(typ, ATypeEnum):
        return to_senum(typ)
    else:
        raise TypeError(f"Unhandled type {type(typ)}")

class TopState:
    def __init__(self):
        self.preproc_line = -1
        self.preproc_start = -1

def top_sdecls(top: ATop, state: TopState = None) -> List[SCommentDecl]:
    if not state:
        state = TopState()
    if isinstance(top, ATopFile):
        decls = []
        for t in top.tops:
            decls += top_sdecls(t, state)
        return decls
    elif isinstance(top, ATopTypedef):
        return [to_sdecl(top.decl, "typedef")]
    elif isinstance(top, ATopExtern):
        return [to_sdecl(top.decl, "extern")]
    elif isinstance(top, ATopDecl):
        return [to_sdecl(top.decl, "toplevel")]
    elif isinstance(top, ATopComment):
        return [to_scomment(top)]
    elif isinstance(top, ATopList):
        line = top.name.location.line
        name = top.name.text()
        st = to_stype(top.type.type)
        st = name_to_stype(st, top.type.names[0])
        return [SDecl(line, line, "list", [SName(None, SType("struct", name,
            body=SStruct(line, "struct", name, [
                SDecl(line, line, "field", [SName("data", st._replace(mods=st.mods+[SModPointer()]))]),
                SDecl(line+1, line+1, "field", [SName("count", SType("name", "size_t"))]),
            ], is_list=True)
        ))])]
    elif isinstance(top, ATopEnumType):
        line = top.enum_type.location.line
        name = top.prefix.text() + "_COUNT"
        return [SDecl(line, line, "enumCount",
            [SName(name, SType("enumType", "enumType", body=SEnumType(
                line, top.enum_type.text(), top.prefix.text(), top.last_value.text())
                )
            )]
        )]
    elif isinstance(top, ATopPreproc):
        line = top.preproc.location.line - 1
        text = top.preproc.text()
        m = re.match(r"#\s*define\s+(\w+)(\([^\)]*\))?\s+(.*)", text)
        if m:
            start_line = line
            if line == state.preproc_line + 1:
                start_line = state.preproc_start
            name = m.group(1)
            args = m.group(2)
            if args:
                args = [arg.strip() for arg in args.split(",")]
            else:
                args = None
            value = m.group(3)
            return [SDecl(start_line, line, "define", [SName(name, SType("define", "define"))],
                define_args=args,
                value=value)]
        else:
            if line != state.preproc_line + 1:
                state.preproc_start = line
            state.preproc_line = line
            return [] # TODO
    else:
        raise TypeError(f"Unhandled type {type(top)}")

def collect_decl_comments(decls: List[SCommentDecl]):
    n = 0
    while n < len(decls):
        dc = decls[n:n+3]
        if isinstance(dc[0], SComment):
            if (len(dc) >= 2 and isinstance(dc[1], SDecl) and dc[0].line_end + 1 == dc[1].line_begin
                and (len(dc) < 3 or not (isinstance(dc[2], SComment) and dc[1].line_end == dc[2].line_begin))):
                yield dc[1]._replace(comment=dc[0])
                n += 2
            else:
                yield dc[0]
                n += 1
        else:
            if len(dc) >= 2 and isinstance(dc[1], SComment) and dc[0].line_end == dc[1].line_begin:
                comment = dc[1]._replace(text=[re.sub(r"^\s*<\s*", "", t) for t in dc[1].text])
                yield dc[0]._replace(comment=comment, comment_inline=True)
                n += 2
            else:
                yield dc[0]
                n += 1

def collect_decl_groups(decls: List[SCommentDecl]):
    n = 0
    while n < len(decls):
        dc = decls[n]
        if isinstance(dc, SDecl) and not dc.comment_inline and not (dc.names and dc.names[0].type.body):
            group = [dc]
            line = dc.line_end + 1
            n += 1
            while n < len(decls):
                dc2 = decls[n]
                if not isinstance(dc2, SDecl): break
                if dc2.comment: break
                if dc2.line_begin != line: break
                if dc2.names and dc2.names[0].type.body: break
                if dc2.is_function != dc.is_function: break
                group.append(dc2)
                line = dc2.line_end + 1
                n += 1
            group[0] = dc._replace(comment=None)
            comment_inline = len(group) == 1 and dc.comment_inline
            yield SDeclGroup(dc.line_begin, group, dc.comment, comment_inline, dc.is_function)
        elif isinstance(dc, SDecl) and not (dc.names and dc.names[0].type.body):
            group = [dc._replace(comment=None)]
            yield SDeclGroup(dc.line_begin, group, dc.comment, dc.comment_inline, dc.is_function)
            n += 1
        else:
            yield dc
            n += 1

def collect_decls(decls: List[SCommentDecl], allow_groups: bool) -> List[SCommentDecl]:
    decls = list(collect_decl_comments(decls))
    if allow_groups:
        decls = list(collect_decl_groups(decls))
    return decls

def format_arg(decl: SDecl):
    name = decl.names[0]
    return {
        "type": format_type(name.type),
        "name": name.name,
    }

def format_mod(mod: SMod):
    if isinstance(mod, SModConst):
        return { "type": "const" }
    elif isinstance(mod, SModNullable):
        return { "type": "nullable" }
    elif isinstance(mod, SModInline):
        return { "type": "inline" }
    elif isinstance(mod, SModAbi):
        return { "type": "abi" }
    elif isinstance(mod, SModAbiData):
        return { "type": "abi_data" }
    elif isinstance(mod, SModPointer):
        return { "type": "pointer" }
    elif isinstance(mod, SModUnsafe):
        return { "type": "unsafe" }
    elif isinstance(mod, SModArray):
        return { "type": "array", "length": mod.length }
    elif isinstance(mod, SModFunction):
        return { "type": "function", "args": [format_arg(d) for d in mod.args] }
    else:
        raise TypeError(f"Unhandled mod {type(mod)}")

def format_type(type: SType):
    return {
        "kind": type.kind,
        "name": type.name,
        "mods": [format_mod(mod) for mod in type.mods],
    }

def format_name(name: SName):
    return {
        "type": format_type(name.type),
        "name": name.name,
    }

def format_decls(decls: List[SCommentDecl], allow_groups: bool):
    for decl in collect_decls(decls, allow_groups):
        if isinstance(decl, SComment):
            yield {
                "kind": "paragraph",
                "comment": decl.text,
            }
        elif isinstance(decl, SDecl):
            body = None
            if decl.names and decl.names[0].type.body:
                body = decl.names[0].type.body
            if isinstance(body, SStruct):
                yield {
                    "kind": "struct",
                    "structKind": body.kind,
                    "line": body.line,
                    "name": body.name,
                    "comment": decl.comment.text if decl.comment else [],
                    "commentInline": decl.comment_inline,
                    "isList": body.is_list,
                    "decls": list(format_decls(body.decls, allow_groups=True)),
                }
            elif isinstance(body, SEnum):
                yield {
                    "kind": "enum",
                    "line": body.line,
                    "name": body.name,
                    "comment": decl.comment.text if decl.comment else [],
                    "commentInline": decl.comment_inline,
                    "decls": list(format_decls(body.decls, allow_groups=True)),
                }
            elif isinstance(body, SEnumType):
                yield {
                    "kind": "enumType",
                    "line": body.line,
                    "enumName": body.enum_name,
                    "countName": body.enum_prefix + "_COUNT",
                    "lastValue": body.last_value,
                    "comment": decl.comment.text if decl.comment else [],
                    "commentInline": decl.comment_inline,
                }
            else:
                for name in decl.names:
                    yield {
                        "kind": "decl",
                        "declKind": decl.kind,
                        "line": decl.line_begin,
                        "name": name.name,
                        "comment": decl.comment.text if decl.comment else [],
                        "commentInline": decl.comment_inline,
                        "isFunction": decl.is_function,
                        "value": decl.value,
                        "defineArgs": decl.define_args,
                        "type": format_type(name.type),
                    }
        elif isinstance(decl, SDeclGroup):
            yield {
                "kind": "group",
                "line": decl.line,
                "name": None,
                "comment": decl.comment.text if decl.comment else [],
                "commentInline": decl.comment_inline,
                "isFunction": decl.is_function,
                "decls": list(format_decls(decl.decls, allow_groups=False)),
            }
        else:
            raise TypeError(f"Unhandled type {type(decl)}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser("ufbx_parser.py")
    parser.add_argument("-i", help="Input file")
    parser.add_argument("-o", help="Output file")
    argv = parser.parse_args()

    src_path = os.path.dirname(os.path.realpath(__file__))

    input_file = argv.i
    if not input_file:
        input_file = os.path.join(src_path, "..", "ufbx.h")

    output_file = argv.o
    if not output_file:
        output_file = os.path.join(src_path, "build", "ufbx.json")

    output_path = os.path.dirname(os.path.realpath(output_file))
    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)
    
    with open(input_file) as f:
        source = f.read()

    p = Parser(source, "ufbx.h")
    top_file = p.parse_top_file()
    result = top_sdecls(top_file)

    js = list(format_decls(result, allow_groups=True))

    with open(output_file, "wt") as f:
        json.dump(js, f, indent=2)
