from typing import NamedTuple, Dict, List, Optional, Union, get_type_hints
import typing
import json
import os

get_origin = getattr(typing, "get_origin", lambda o: getattr(o, "__origin__", None))
get_args = getattr(typing, "get_args", lambda o: getattr(o, "__args__", None))

class BaseField(NamedTuple):
    name: str
    json_name: str
    type: type
    optional: bool
    list: bool
    dict: bool

def make_field(name, base):
    json_name = "".join((p.title() if i > 0 else p) for i,p in enumerate(name.split("_")))
    origin, args = get_origin(base), get_args(base)
    optional = list_ = dict_ = False
    if origin == Union and len(args) == 2 and type(None) in args:
        base = args[args.index(type(None)) ^ 1]
        optional = True
        origin, args = get_origin(base), get_args(base)
    if origin in (List, list):
        base = args[0]
        list_ = True
    if origin in (Dict, dict) and len(args) == 2 and args[0] == str:
        base = args[1]
        dict_ = True
    return BaseField(name, json_name, base, optional, list_, dict_)

def get_fields(cls):
    if not hasattr(cls, "_base_fields"):
        cls._base_fields = [make_field(name, typ) for name, typ in get_type_hints(cls).items()]
    return cls._base_fields

def from_json(typ, json_obj):
    if typ in (str, int, float, bool):
        if not isinstance(json_obj, typ):
            raise ValueError(f"Wrong type for {typ.__name__}, got {type(json_obj)} expected {typ}")
        return json_obj

    values = { }
    for field in get_fields(typ):
        json_field = json_obj.get(field.json_name)
        if not field.optional and json_field is None:
            raise ValueError(f"Missing non-optional field {typ.__name__}.{field.json_name}")
        if json_field is None:
            values[field.name] = None
            continue

        if field.list:
            if not isinstance(json_field, list):
                raise ValueError(f"Wrong type for {typ.__name__}.{field.json_name}, got {type(json_obj)} expected a list of {field.type}")
            values[field.name] = [from_json(field.type, js) for js in json_field]
        elif field.dict:
            if not isinstance(json_field, dict):
                raise ValueError(f"Wrong type for {typ.__name__}.{field.json_name}, got {type(json_obj)} expected an object containing {field.type}")
            values[field.name] = { k: from_json(field.type, js) for k,js in json_field.items() }
        else:
            values[field.name] = from_json(field.type, json_field)
    return typ(**values)

def to_json_imp(typ, obj):
    if typ in (str, int, float, bool):
        if not isinstance(obj, typ):
            raise TypeError(f"Wrong type for {typ.__name__}, got {type(obj)} expected {typ}")
        return obj

    values = { }
    for field in get_fields(typ):
        obj_field = getattr(obj, field.name, None)
        if not field.optional and obj_field is None:
            raise TypeError(f"Missing non-optional field {typ.__name__}.{field.name}")
        if obj_field is None:
            values[field.json_name] = None
            continue

        if field.list:
            if not isinstance(obj_field, list):
                raise TypeError(f"Wrong type for {typ.__name__}.{field.name}, got {type(obj_field)} expected a list of {field.type}")
            values[field.json_name] = [to_json_imp(field.type, js) for js in obj_field]
        elif field.dict:
            if not isinstance(obj_field, dict):
                raise TypeError(f"Wrong type for {typ.__name__}.{field.name}, got {type(obj_field)} expected an object containing {field.type}")
            values[field.json_name] = { k: to_json_imp(field.type, ob) for k,ob in obj_field.items() }
        else:
            values[field.json_name] = to_json_imp(field.type, obj_field)
    return values

def to_json(obj):
    return to_json_imp(type(obj), obj)

class Base:
    def __init__(self, **kwargs):
        for field in get_fields(type(self)):
            val = kwargs.get(field.name, None)
            if not field.optional and val is None:
                if field.list:
                    val = []
                elif field.dict:
                    val = { }
                else:
                    val = field.type()
            setattr(self, field.name, val)

    def __repr__(self):
        cls = type(self)
        fs = ", ".join(f"{f.name}={getattr(self, f.name, None)!r}" for f in get_fields(cls))
        return f"{cls.__name__}({fs})"

class Mod(Base):
    pass

class Constant(Base):
    name: str
    value_int: int

class Type(Base):
    key: str
    base_name: str
    size: Dict[str, int]
    align: Dict[str, int]
    kind: str
    is_nullable: bool
    is_const: bool
    is_pod: bool
    is_function: bool
    array_length: Optional[int]
    func_args: List["Argument"]
    inner: Optional[str]

class Field(Base):
    type: str
    name: str    
    kind: str
    private: bool
    offset: Dict[str, int]
    comment: Optional[str]
    union_sized: Dict[str, bool]
    union_preferred: bool

class Struct(Base):
    name: str
    short_name: str
    fields: List[Field]
    comment: Optional[str]
    vertex_attrib_type: Optional[str]
    is_union: bool
    is_anonymous: bool
    is_list: bool
    is_element: bool
    is_pod: bool
    is_input: bool
    is_callback: bool
    is_interface: bool
    member_functions: List[str]
    member_globals: List[str]

class EnumValue(Base):
    name: str    
    short_name: str
    short_name_raw: str
    value: int
    comment: Optional[str]
    flag: bool
    auxiliary: bool

class Enum(Base):
    name: str    
    short_name: str
    values: List[str]
    flag: bool

class Argument(Base):
    name: str
    type: str
    kind: str
    is_return: bool
    by_ref: bool
    return_ref: bool

class StringArgument(Base):
    name: str
    pointer_index: int
    length_index: int

class ArrayArgument(Base):
    name: str
    pointer_index: int
    num_index: int

class BlobArgument(Base):
    name: str
    pointer_index: int
    size_index: int

class Function(Base):
    name: str
    short_name: str
    pretty_name: str
    return_type: str
    return_kind: str
    kind: str
    arguments: List[Argument]
    string_arguments: List[StringArgument]
    array_arguments: List[ArrayArgument]
    blob_arguments: List[BlobArgument]
    is_inline: bool
    is_unsafe: bool
    member_name: Optional[str]
    ffi_name: Optional[str]
    catch_name: Optional[str]
    len_name: Optional[str]
    is_ffi: bool
    is_catch: bool
    is_len: bool
    has_error: bool
    has_panic: bool
    alloc_type: str
    return_array_scale: int
    nullable_return: bool

class Global(Base):
    name: str
    short_name: str
    type: str

class Typedef(Base):
    name: str
    short_name: str
    type: str

class Declaration(Base):
    kind: str
    name: str

class MemberFunction(Base):
    func: str
    self_type: str
    member_name: str
    self_index: int

class MemberGlobal(Base):
    global_name: str
    self_type: str
    member_name: str

class File(Base):
    constants: Dict[str, Constant]
    types: Dict[str, Type]
    structs: Dict[str, Struct]
    enums: Dict[str, Enum]
    enum_values: Dict[str, EnumValue]
    functions: Dict[str, Function]
    globals: Dict[str, Global]
    typedefs: Dict[str, Typedef]
    member_functions: Dict[str, MemberFunction]
    member_globals: Dict[str, MemberGlobal]
    declarations: List[Declaration]
    element_types: List[str]

def init_type(file, typ, key, mods):
    name = typ["name"]
    t = Type(base_name=name, key=key)

    if typ.get("kind", "") == "define":
        t.kind = "define"

    if mods:
        mods = mods[:]

        if mods[0]["type"] == "nullable":
            if mods[-1]["type"] == "pointer":
                mods[-1]["nullable"] = True
                mods = mods[1:]

        if mods[0]["type"] == "const":
            if mods[-1]["type"] == "pointer":
                mods[-1]["const"] = True
                mods = mods[1:]

    if mods:
        mod = mods[-1]
        mt = mod["type"]
        mods = mods[:-1]

        if mt == "pointer":
            t.kind = "pointer"
            t.is_nullable = mod.get("nullable", False)
            t.is_const = mod.get("const", False)
            t.inner = parse_type_imp(file, typ, mods)
        elif mt == "array":
            length = eval_const(file, mod["length"])
            t.kind = "array"
            t.array_length = length
            t.inner = parse_type_imp(file, typ, mods)
        elif mt == "function":
            t.kind = "function"
            t.func_args = [parse_argument(file, arg) for arg in mod["args"]]
            t.inner = parse_type_imp(file, typ, mods)
        elif mt == "const":
            t.kind = "const"
            t.inner = parse_type_imp(file, typ, mods)
        elif mt == "unsafe":
            t.kind = "unsafe"
            t.inner = parse_type_imp(file, typ, mods)
        else:
            raise ValueError(f"Unhandled mod {mt}")

    return t

def eval_const(file, expr):
    if expr in file.constants:
        return file.constants[expr].value_int
    elif expr in file.enum_values:
        return file.enum_values[expr].value
    else:
        return int(expr, base=0)

def parse_type_imp(file, typ, mods):
    key = typ["name"]
    for mod in mods:
        mt = mod["type"]
        if mt == "pointer":
            key = key + "*"
        elif mt == "nullable":
            key = key + "?"
        elif mt == "unsafe":
            key = key + " unsafe"
        elif mt == "const":
            key = key + " const"
        elif mt == "array":
            length = eval_const(file, mod["length"])
            key = key + f"[{length}]"
        elif mt == "function":
            args = [parse_type(file, a["type"]) for a in mod["args"]]
            args_str = ",".join(args)
            key = key + f"({args_str})"
        else:
            raise ValueError(f"Unhandled mod {mt}")

    if key not in file.types:
        file.types[key] = init_type(file, typ, key, mods)

    return key

def parse_type(file, typ, in_func=False):
    mods = typ["mods"]
    if in_func:
        mods = [m for m in mods if m["type"] not in ("function", "inline", "abi", "unsafe")]
    else:
        mods = [m for m in mods if m["type"] not in ("abi_data", )]
    return parse_type_imp(file, typ, mods)

def parse_field(file: File, st: Struct, decl, anon_path):
    anon_id = 0

    kind, name = decl["kind"], decl.get("name")
    if kind == "group":
        for inner in decl["decls"]:
            parse_field(file, st, inner, anon_path)
    elif kind == "struct":
        anon_name = f"{anon_path}.{anon_id}"
        anon_id += 1
        ast = Struct(name=anon_name)
        ast.is_anonymous = True
        at = Type(base_name=anon_name, key=anon_name)
        at.kind = "struct"
        file.types[anon_name] = at
        file.structs[anon_name] = ast
        if decl["structKind"] == "union":
            ast.is_union = True
        for inner in decl["decls"]:
            parse_field(file, ast, inner, anon_name)
        fd = Field(name="", type=anon_name)
        st.fields.append(fd)
    elif kind == "decl":
        typ = parse_type(file, decl["type"])
        fd = Field(name=name, type=typ)
        if name.startswith("_"):
            fd.private = True
        st.fields.append(fd)

def postprocess_fields(file: File, st: Struct):
    for field in st.fields:
        tp = file.types[field.type]
        if tp.kind == "array" and tp.inner == "char":
            len_ix = find_index(st.fields, lambda f: f.name == field.name + "_length")
            if len_ix >= 0:
                field.kind = "inlineBuf"
                st.fields[len_ix].kind = "inlineBufLength"

def shorten_name(name: str, prefix: str):
    for part in prefix.split("_"):
        if name.lower().startswith(part.lower() + "_"):
            name = name[len(part)+1:]
    if prefix.lower().endswith("_flags") and name.lower().startswith("flag_"):
        name = name[5:]
    return name

def shorten_global(name: str):
    if name.lower().startswith("ufbx_"):
        name = name[5:]
    return name

class EnumCtx:
    def __init__(self):
        self.next_value = 0
        self.hit_aux = False

def parse_enum(file: File, en: Enum, decl, ctx):
    kind, name = decl["kind"], decl.get("name")
    if kind == "group":
        for inner in decl["decls"]:
            parse_enum(file, en, inner, ctx)
    elif kind == "decl":
        if name in ("UFBX_ELEMENT_TYPE_FIRST_ATTRIB"):
            ctx.hit_aux = True
        ev = EnumValue(name=name, flag=en.flag)
        ev.short_name_raw = shorten_name(name, en.name)
        ev.short_name = ev.short_name_raw
        ev.auxiliary = ctx.hit_aux
        if ev.short_name[0].isdigit():
            ev.short_name = "E" + ev.short_name
        val = decl.get("value")
        if val:
            ev.value = eval_const(file, val)
        else:
            ev.value = ctx.next_value
        ctx.next_value = ev.value + 1
        en.values.append(name)
        file.enum_values[name] = ev

def parse_argument(file: File, arg):
    name = arg["name"]
    typ = parse_type(file, arg["type"])
    return Argument(name=name, type=typ, kind="")

def parse_func(file: File, decl):
    name = decl["name"]
    mods = decl["type"]["mods"]
    func_mod = next(m for m in mods if m["type"] == "function")

    fn = Function(name=name)
    fn.short_name = shorten_global(name)
    fn.pretty_name = fn.short_name
    if fn.pretty_name.endswith("_len"):
        fn.pretty_name = fn.pretty_name[:-4]
    fn.return_type = parse_type(file, decl["type"], in_func=True)
    fn.is_inline = any(m for m in mods if m["type"] == "inline")
    fn.is_unsafe = any(m for m in mods if m["type"] == "unsafe")
    fn.arguments = [parse_argument(file, arg) for arg in func_mod["args"]]

    file.functions[name] = fn
    file.declarations.append(Declaration(kind="function", name=name))

def parse_global(file: File, decl):
    name = decl["name"]

    gl = Global(name=name)
    gl.short_name = shorten_global(name)
    gl.type = parse_type(file, decl["type"])
    file.globals[name] = gl
    file.declarations.append(Declaration(kind="global", name=name))

def parse_typedef(file: File, decl):
    name = decl["name"]

    td = Typedef(name=name)
    td.short_name = shorten_global(name)
    td.type = parse_type(file, decl["type"])
    file.typedefs[name] = td
    file.declarations.append(Declaration(kind="typedef", name=name))

    t = Type(name=name, base_name=name)
    t.kind = "typedef"
    t.key = name
    t.inner = td.type
    file.types[name] = t

def parse_decl(file: File, decl):
    kind, name = decl["kind"], decl.get("name")
    if kind == "group":
        for inner in decl["decls"]:
            parse_decl(file, inner)
    elif kind == "struct":
        st = Struct(name=name)
        st.short_name = shorten_global(name)
        t = Type(base_name=name, key=name)
        t.kind = "struct"
        file.structs[name] = st
        file.types[name] = t
        file.declarations.append(Declaration(kind="struct", name=name))
        if decl.get("isList"):
            st.is_list = True
        for inner in decl["decls"]:
            parse_field(file, st, inner, name)
    elif kind == "enum":
        en = Enum(name=name)
        en.short_name = shorten_global(name)
        t = Type(base_name=name, key=name)
        t.kind = "enum"

        if name.endswith("_flags"):
            en.flag = True

        file.enums[name] = en
        file.types[name] = t
        file.declarations.append(Declaration(kind="enum", name=name))
        ctx = EnumCtx()
        for inner in decl["decls"]:
            parse_enum(file, en, inner, ctx)
    elif kind == "decl":
        if decl["declKind"] == "typedef":
            if decl["type"]["name"] != decl["name"]:
                parse_typedef(file, decl)
        elif decl["isFunction"]:
            parse_func(file, decl)
        elif decl["kind"] == "decl":
            parse_global(file, decl)
    elif kind == "enumType":
        line = decl["line"]
        enum_name = decl["enumName"]
        last_value = decl["lastValue"]
        count_name = decl["countName"]
        en = file.enums.get(enum_name)
        if not en:
            raise RuntimeError(f"ufbx.h:{line}: UFBX_ENUM_TYPE() has undefined enum name {enum_name}")
        max_value = max((file.enum_values[n] for n in en.values), key=lambda v: v.value)
        if max_value.name != last_value:
            if last_value in file.enum_values:
                wrong_value = file.enum_values[last_value].value
            else:
                wrong_value = "(undefined)"
            raise RuntimeError(f"ufbx.h:{line}: UFBX_ENUM_TYPE() has wrong highest value ({last_value} = {wrong_value}), actual highest value is ({max_value.name} = {max_value.value})")
        count = max_value.value + 1
        file.constants[count_name] = Constant(name=count_name, value_int=count)
        file.declarations.append(Declaration(kind="enumCount", name=count_name))

def parse_file(decls):
    file = File()

    # HACK
    file.constants["UFBX_ERROR_STACK_MAX_DEPTH"] = Constant(name="UFBX_ERROR_STACK_MAX_DEPTH", value_int=8)
    file.constants["UFBX_PANIC_MESSAGE_LENGTH"] = Constant(name="UFBX_PANIC_MESSAGE_LENGTH", value_int=128)
    file.constants["UFBX_ERROR_INFO_LENGTH"] = Constant(name="UFBX_ERROR_INFO_LENGTH", value_int=256)

    for decl in decls:
        parse_decl(file, decl)
    return file

class Arch:
    def __init__(self, name, sizes):
        self.name = name
        self.sizes = sizes

sizes_base = {
    "void": 0,
    "char": 1,
    "bool": 1,
    "uint8_t": 1,
    "int8_t": 1,
    "uint16_t": 2,
    "int16_t": 2,
    "uint32_t": 4,
    "int32_t": 4,
    "uint64_t": 8,
    "int64_t": 8,
    "float": 4,
    "double": 8,
    "enum": 4,
    "UFBX_REAL_TYPE": 8,
}

sizes_32bit = {
    **sizes_base,
    "size_t": 4,
    "ptrdiff_t": 4,
    "uintptr_t": 4,
    "*": 4,
}

sizes_64bit = {
    **sizes_base,
    "size_t": 8,
    "ptrdiff_t": 8,
    "uintptr_t": 8,
    "*": 8,
}

archs = [
    Arch("x86", sizes_32bit),
    Arch("x64", sizes_64bit),
    Arch("wasm", sizes_32bit),
]

def layout_struct(arch: Arch, file: File, typ: Type, st: Struct):
    offset = 0
    align = 0
    union_size = 0
    for field in st.fields:
        field_type = file.types[field.type]
        layout_type(arch, file, field_type)
        field_size = field_type.size[arch.name]
        field_align = field_type.align[arch.name]

        align = max(align, field_align)
        while offset % field_align != 0:
            offset += 1

        field.offset[arch.name] = offset

        if st.is_union:
            union_size = max(union_size, field_size)
        else:
            offset += field_size

    while offset % align != 0:
        offset += 1

    if st.is_union:
        offset = union_size
        for field in st.fields:
            field_type = file.types[field.type]
            field_size = field_type.size[arch.name]
            if field_size == union_size:
                field.union_sized[arch.name] = True

    typ.size[arch.name] = offset
    typ.align[arch.name] = align

def layout_type(arch: Arch, file: File, typ: Type):
    if arch.name in typ.size:
        return

    if typ.kind == "pointer":
        size = arch.sizes["*"]
        typ.size[arch.name] = size
        typ.align[arch.name] = size
    elif typ.kind == "":
        size = arch.sizes[typ.base_name]
        typ.size[arch.name] = size
        typ.align[arch.name] = size
    elif typ.kind == "enum":
        size = arch.sizes["enum"]
        typ.size[arch.name] = size
        typ.align[arch.name] = size
    elif typ.kind == "typedef":
        inner = file.types[typ.inner]
        layout_type(arch, file, inner)
        typ.size[arch.name] = inner.size[arch.name]
        typ.align[arch.name] = inner.align[arch.name]
    elif typ.kind in ("const", "unsafe"):
        inner = file.types[typ.inner]
        layout_type(arch, file, inner)
        typ.size[arch.name] = inner.size[arch.name]
        typ.align[arch.name] = inner.align[arch.name]
    elif typ.kind == "struct":
        st = file.structs[typ.base_name]
        layout_struct(arch, file, typ, st)
    elif typ.kind == "array":
        inner = file.types[typ.inner]
        layout_type(arch, file, inner)
        typ.size[arch.name] = inner.size[arch.name] * typ.array_length
        typ.align[arch.name] = inner.align[arch.name]
    elif typ.kind == "function":
        inner = file.types[typ.inner]
        layout_type(arch, file, inner)
        typ.size[arch.name] = 0
        typ.align[arch.name] = 0
    elif typ.kind == "define":
        typ.size[arch.name] = 0
        typ.align[arch.name] = 0
    else:
        raise ValueError(f"Unhandled type kind: {typ.kind}")

def layout_file(arch: Arch, file: File):
    for typ in file.types.values():
        layout_type(arch, file, typ)

def to_pascal(name_snake):
    parts = name_snake.lower().split("_")
    for n in range(0, len(parts)):
        parts[n] = parts[n].title()
    return "".join(parts)

def to_camel(name_snake):
    parts = name_snake.lower().split("_")
    for n in range(1, len(parts)):
        parts[n] = parts[n].title()
    return "".join(parts)

prim_types = {
    "bool",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t",
    "size_t",
    "float",
    "double",
    "uintptr_t",
}

pod_types = {
    *prim_types,
}

ref_types = {
    "ufbx_scene",
    "ufbx_element",
    "ufbx_anim",
    "ufbx_props",
    "ufbx_vertex_real",
    "ufbx_vertex_vec2",
    "ufbx_vertex_vec3",
    "ufbx_vertex_vec4",
    "ufbx_geometry_cache",
    "ufbx_cache_channel",
    "ufbx_cache_frame",
    "ufbx_texture_file",
}

pod_structs = [
    "ufbx_vec2",
    "ufbx_vec3",
    "ufbx_vec4",
    "ufbx_quat",
    "ufbx_matrix",
    "ufbx_transform",
    "ufbx_edge",
    "ufbx_face",
    "ufbx_lod_level",
    "ufbx_skin_vertex",
    "ufbx_skin_weight",
    "ufbx_tangent",
    "ufbx_keyframe",
    "ufbx_curve_point",
    "ufbx_surface_point",
    "ufbx_topo_edge",
    "ufbx_coordinate_axes",
    "ufbx_transform_override",
]

input_structs = [
    "ufbx_allocator_opts",
    "ufbx_open_memory_opts",
    "ufbx_open_file_opts",
    "ufbx_load_opts",
    "ufbx_evaluate_opts",
    "ufbx_tessellate_curve_opts",
    "ufbx_tessellate_surface_opts",
    "ufbx_subdivide_opts",
    "ufbx_geometry_cache_opts",
    "ufbx_geometry_cache_data_opts",
    "ufbx_anim_opts",
    "ufbx_prop_override_desc",
    "ufbx_bake_opts",
    "ufbx_thread_opts",
]

interface_structs = [
    "ufbx_allocator",
    "ufbx_stream",
    "ufbx_thread_pool",
]

union_prefer = {
    "ufbx_vec2.0": 0,
    "ufbx_vec3.0": 0,
    "ufbx_vec4.0": 0,
    "ufbx_quat.0": 0,
    "ufbx_matrix.0": 0,
    "ufbx_scene.0": 0,
    "ufbx_edge.0": 0,
}

member_functions = [
    MemberFunction(func="ufbx_find_prop_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_prop", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_real_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_real", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_vec3_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_vec3", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_int_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_int", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_bool_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_bool", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_string_len", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_string", self_type="ufbx_props"),
    MemberFunction(func="ufbx_find_element_len", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_element", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_node_len", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_node", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_anim_stack_len", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_anim_stack", self_type="ufbx_scene"),
    MemberFunction(func="ufbx_find_anim_prop_len", self_type="ufbx_anim_layer"),
    MemberFunction(func="ufbx_find_anim_prop", self_type="ufbx_anim_layer"),
    MemberFunction(func="ufbx_find_anim_props", self_type="ufbx_anim_layer"),
    MemberFunction(func="ufbx_get_compatible_matrix_for_normals", self_type="ufbx_node"),
    MemberFunction(func="ufbx_evaluate_curve", self_type="ufbx_anim_curve", member_name="evaluate"),
    MemberFunction(func="ufbx_evaluate_anim_value_real", self_type="ufbx_anim_value", member_name="evaluate_real"),
    # MemberFunction(func="ufbx_evaluate_anim_value_vec2", self_type="ufbx_anim_value", member_name="evaluate_vec2"),
    MemberFunction(func="ufbx_evaluate_anim_value_vec3", self_type="ufbx_anim_value", member_name="evaluate_vec3"),
    MemberFunction(func="ufbx_evaluate_transform", self_type="ufbx_node"),
    MemberFunction(func="ufbx_evaluate_blend_weight", self_type="ufbx_blend_channel"),
    MemberFunction(func="ufbx_evaluate_scene", self_type="ufbx_scene", member_name="evaluate"),
    MemberFunction(func="ufbx_find_prop_texture_len", self_type="ufbx_material"),
    MemberFunction(func="ufbx_find_prop_texture", self_type="ufbx_material"),
    MemberFunction(func="ufbx_find_shader_prop_len", self_type="ufbx_shader"),
    MemberFunction(func="ufbx_find_shader_prop", self_type="ufbx_shader"),
    MemberFunction(func="ufbx_catch_get_skin_vertex_matrix", self_type="ufbx_skin_deformer"),
    MemberFunction(func="ufbx_get_skin_vertex_matrix", self_type="ufbx_skin_deformer"),
    MemberFunction(func="ufbx_get_blend_shape_vertex_offset", self_type="ufbx_blend_shape", member_name="get_vertex_offset"),
    MemberFunction(func="ufbx_get_blend_vertex_offset", self_type="ufbx_blend_deformer", member_name="get_vertex_offset"),
    MemberFunction(func="ufbx_add_blend_shape_vertex_offsets", self_type="ufbx_blend_shape", member_name="add_vertex_offsets"),
    MemberFunction(func="ufbx_add_blend_vertex_offsets", self_type="ufbx_blend_deformer", member_name="add_vertex_offsets"),
    MemberFunction(func="ufbx_evaluate_nurbs_basis", self_type="ufbx_nurbs_basis", member_name="evaluate"),
    MemberFunction(func="ufbx_evaluate_nurbs_curve", self_type="ufbx_nurbs_curve", member_name="evaluate"),
    MemberFunction(func="ufbx_evaluate_nurbs_surface", self_type="ufbx_nurbs_surface", member_name="evaluate"),
    MemberFunction(func="ufbx_tessellate_nurbs_curve", self_type="ufbx_nurbs_curve", member_name="tessellate"),
    MemberFunction(func="ufbx_tessellate_nurbs_surface", self_type="ufbx_nurbs_surface", member_name="tessellate"),
    MemberFunction(func="ufbx_catch_triangulate_face", self_type="ufbx_mesh"),
    MemberFunction(func="ufbx_triangulate_face", self_type="ufbx_mesh"),
    MemberFunction(func="ufbx_subdivide_mesh", self_type="ufbx_mesh", member_name="subdivide"),
    MemberFunction(func="ufbx_read_geometry_cache_real", self_type="ufbx_cache_frame", member_name="read_real"),
    MemberFunction(func="ufbx_sample_geometry_cache_real", self_type="ufbx_cache_channel", member_name="sample_real"),
    MemberFunction(func="ufbx_read_geometry_cache_vec3", self_type="ufbx_cache_frame", member_name="read_vec3"),
    MemberFunction(func="ufbx_sample_geometry_cache_vec3", self_type="ufbx_cache_channel", member_name="sample_vec3"),
    MemberFunction(func="ufbx_dom_find_len", self_type="ufbx_dom_node", member_name="find_len"),
    MemberFunction(func="ufbx_dom_find", self_type="ufbx_dom_node", member_name="find"),
    MemberFunction(func="ufbx_dom_is_array", self_type="ufbx_dom_node", member_name="is_array"),
    MemberFunction(func="ufbx_dom_array_size", self_type="ufbx_dom_node", member_name="array_size"),
    MemberFunction(func="ufbx_dom_as_int32_list", self_type="ufbx_dom_node", member_name="as_int32_list"),
    MemberFunction(func="ufbx_dom_as_int64_list", self_type="ufbx_dom_node", member_name="as_int64_list"),
    MemberFunction(func="ufbx_dom_as_float_list", self_type="ufbx_dom_node", member_name="as_float_list"),
    MemberFunction(func="ufbx_dom_as_double_list", self_type="ufbx_dom_node", member_name="as_double_list"),
    MemberFunction(func="ufbx_dom_as_real_list", self_type="ufbx_dom_node", member_name="as_real_list"),
    MemberFunction(func="ufbx_dom_as_blob_list", self_type="ufbx_dom_node", member_name="as_blob_list"),
]

member_globals = [
    MemberGlobal(global_name="ufbx_empty_string", self_type="ufbx_string", member_name="empty"),
    MemberGlobal(global_name="ufbx_empty_blob", self_type="ufbx_blob", member_name="empty"),
    MemberGlobal(global_name="ufbx_identity_matrix", self_type="ufbx_matrix", member_name="identity"),
    MemberGlobal(global_name="ufbx_identity_transform", self_type="ufbx_transform", member_name="identity"),
    MemberGlobal(global_name="ufbx_zero_vec2", self_type="ufbx_vec2", member_name="zero"),
    MemberGlobal(global_name="ufbx_zero_vec3", self_type="ufbx_vec3", member_name="zero"),
    MemberGlobal(global_name="ufbx_zero_vec4", self_type="ufbx_vec4", member_name="zero"),
    MemberGlobal(global_name="ufbx_identity_quat", self_type="ufbx_quat", member_name="identity"),
    MemberGlobal(global_name="ufbx_axes_right_handed_y_up", self_type="ufbx_coordinate_axes", member_name="right_handed_y_up"),
    MemberGlobal(global_name="ufbx_axes_right_handed_z_up", self_type="ufbx_coordinate_axes", member_name="right_handed_z_up"),
    MemberGlobal(global_name="ufbx_axes_left_handed_y_up", self_type="ufbx_coordinate_axes", member_name="left_handed_y_up"),
    MemberGlobal(global_name="ufbx_axes_left_handed_z_up", self_type="ufbx_coordinate_axes", member_name="left_handed_z_up"),
]

def find_index(list, predicate):
    for i,v in enumerate(list):
        if predicate(v):
            return i
    return -1

if __name__ == "__main__":
    src_path = os.path.dirname(os.path.realpath(__file__))
    path = os.path.join(src_path, "build", "ufbx.json")
    with open(path, "rt") as f:
        js = json.load(f)
    file = parse_file(js)

    for name in file.enums["ufbx_element_type"].values:
        ev = file.enum_values[name]
        if ev.auxiliary: continue
        name = name.lower().replace("ufbx_element_", "ufbx_")
        st = file.structs[name]
        st.is_element = True
        file.element_types.append(name)
        ref_types.add(name)

    file.structs["ufbx_vertex_real"].vertex_attrib_type = "ufbx_real"
    file.structs["ufbx_vertex_vec2"].vertex_attrib_type = "ufbx_vec2"
    file.structs["ufbx_vertex_vec3"].vertex_attrib_type = "ufbx_vec3"
    file.structs["ufbx_vertex_vec4"].vertex_attrib_type = "ufbx_vec4"

    for pod in pod_structs:
        file.structs[pod].is_pod = True

    for inp in input_structs:
        file.structs[inp].is_input = True

    for inp in interface_structs:
        file.structs[inp].is_interface = True

    for st in file.structs.values():
        if st.name.endswith("_cb"):
            st.is_callback = True

    for name, index in union_prefer.items():
        st = file.structs[name]
        st.fields[index].union_preferred = True

    for st in file.structs.values():
        if not st.is_union: continue
        if any(f.union_preferred for f in st.fields): continue
        st.fields[-1].union_preferred = True

    for typ in file.types.values():
        if typ.kind == "struct":
            st = file.structs[typ.base_name]
            if st.is_pod:
                typ.is_pod = True
        elif typ.kind == "":
            if typ.base_name in pod_types:
                typ.is_pod = True

    for typ in file.types.values():
        if typ.kind == "function":
            typ.is_function = True

    for typ in file.types.values():
        if typ.kind == "typedef":
            inner = file.types[typ.inner]
            if inner.is_pod:
                typ.is_pod = True
            if inner.is_function:
                typ.is_function = True

    for typ in file.types.values():
        if typ.kind == "pointer":
            inner = file.types[typ.inner]
            if inner.is_function:
                typ.is_function = True

    for st in file.structs.values():
        postprocess_fields(file, st)

    for func in file.functions.values():
        for index, arg in enumerate(func.arguments):
            if arg.kind: continue
            if arg.type == "char const*":
                len_name = arg.name + "_len"
                len_index = find_index(func.arguments, lambda a: a.name == len_name)
                if len_index >= 0:
                    arg.kind = "stringPointer"
                    func.arguments[len_index].kind = "stringLength"
                    sa = StringArgument(name=arg.name, pointer_index=index, len_index=len_index)
                    func.string_arguments.append(sa)
                    continue

            typ = file.types[arg.type]
            if arg.name == "retval":
                arg.is_return = True
                assert typ.kind == "pointer"
                inner = file.types[typ.inner]
                if inner.is_pod:
                    arg.kind = "pod"
                    func.return_kind = "pod"
            elif typ.kind == "pointer":
                inner = file.types[typ.inner]

                num_name = "num_" + arg.name
                num_index = find_index(func.arguments, lambda a: a.name == num_name)

                size_name = arg.name + "_size"
                size_index = find_index(func.arguments, lambda a: a.name == size_name)

                if num_index < 0 and inner.key == "char":
                    num_index = size_index

                if num_index >= 0:
                    arg.kind = "arrayPointer"
                    func.arguments[num_index].kind = "arrayLength"
                    aa = ArrayArgument(name=arg.name, pointer_index=index, num_index=num_index)
                    func.array_arguments.append(aa)
                elif size_index >= 0 and inner.key == "void":
                    arg.kind = "blobPointer"
                    func.arguments[size_index].kind = "blobSize"
                    ba = BlobArgument(name=arg.name, pointer_index=index, size_index=size_index)
                    func.blob_arguments.append(ba)
                elif typ.is_const and inner.is_pod:
                    arg.by_ref = True
                    arg.kind = "pod"
                elif typ.is_const and inner.key in input_structs:
                    arg.by_ref = True
                    arg.kind = "input"
                elif not typ.is_const and inner.key == "ufbx_error":
                    arg.by_ref = True
                    arg.kind = "error"
                    func.has_error = True
                elif not typ.is_const and inner.key == "ufbx_panic":
                    arg.by_ref = True
                    arg.kind = "panic"
                    func.has_panic = True
                elif typ.is_const and inner.key == "ufbx_stream":
                    arg.by_ref = True
                    arg.kind = "stream"
                elif typ.is_const and inner.key in ref_types:
                    arg.kind = "ref"
            elif typ.is_pod:
                if arg.type in prim_types:
                    arg.kind = "prim"
                else:
                    arg.kind = "pod"
            elif typ.kind == "enum":
                arg.kind = "enum"
        
        rtyp = file.types[func.return_type]
        if rtyp.kind == "enum":
            func.return_kind = "enum"
        elif func.return_type in prim_types:
            func.return_kind = "prim"
        elif rtyp.kind == "pointer":
            inner = file.types[rtyp.inner]
            if inner.key in ref_types:
                func.return_kind = "ref"

    for func in file.functions.values():
        for index, arg in enumerate(func.arguments):
            if arg.name == "retval": continue
            typ = file.types[arg.type]
            if typ.base_name in file.element_types:
                arg.return_ref = True
            if typ.base_name in { "ufbx_scene", "ufbx_anim", "ufbx_element", "ufbx_geometry_cache", "ufbx_props" }:
                arg.return_ref = True
        if len(func.arguments) == 1 and func.arguments[0].type == "void":
            func.arguments.clear()

    for func in file.functions.values():
        if "_ffi_" in func.name:
            func.is_ffi = True
            non_ffi = func.name.replace("_ffi_", "_", 1)
            file.functions[non_ffi].ffi_name = func.name
        elif "_catch_" in func.name:
            func.is_catch = True
            non_catch = func.name.replace("_catch_", "_", 1)
            file.functions[non_catch].catch_name = func.name
        elif func.name.endswith("_len"):
            func.is_len = True
            non_len = func.name[:-4]
            file.functions[non_len].len_name = func.name

    file.functions["ufbx_load_file"].alloc_type = "scene"
    file.functions["ufbx_load_file_len"].alloc_type = "scene"
    file.functions["ufbx_load_memory"].alloc_type = "scene"
    file.functions["ufbx_load_stream"].alloc_type = "scene"
    file.functions["ufbx_load_stream_prefix"].alloc_type = "scene"
    file.functions["ufbx_load_stdio"].alloc_type = "scene"
    file.functions["ufbx_load_stdio_prefix"].alloc_type = "scene"
    file.functions["ufbx_evaluate_scene"].alloc_type = "scene"
    file.functions["ufbx_subdivide_mesh"].alloc_type = "mesh"
    file.functions["ufbx_tessellate_nurbs_curve"].alloc_type = "line"
    file.functions["ufbx_tessellate_nurbs_surface"].alloc_type = "mesh"
    file.functions["ufbx_load_geometry_cache"].alloc_type = "geometryCache"
    file.functions["ufbx_load_geometry_cache_len"].alloc_type = "geometryCache"
    file.functions["ufbx_create_anim"].alloc_type = "anim"
    file.functions["ufbx_bake_anim"].alloc_type = "bakedAnim"

    file.functions["ufbx_free_scene"].kind = "free"
    file.functions["ufbx_free_mesh"].kind = "free"
    file.functions["ufbx_free_line_curve"].kind = "free"
    file.functions["ufbx_free_geometry_cache"].kind = "free"
    file.functions["ufbx_free_anim"].kind = "free"
    file.functions["ufbx_free_baked_anim"].kind = "free"

    file.functions["ufbx_retain_scene"].kind = "retain"
    file.functions["ufbx_retain_mesh"].kind = "retain"
    file.functions["ufbx_retain_line_curve"].kind = "retain"
    file.functions["ufbx_retain_geometry_cache"].kind = "retain"
    file.functions["ufbx_retain_anim"].kind = "retain"
    file.functions["ufbx_retain_baked_anim"].kind = "retain"

    file.functions["ufbx_triangulate_face"].return_array_scale = 3
    # file.functions["ufbx_ffi_triangulate_face"].return_array_scale = 3
    file.functions["ufbx_read_geometry_cache_real"].return_array_scale = 1
    file.functions["ufbx_sample_geometry_cache_real"].return_array_scale = 1
    file.functions["ufbx_read_geometry_cache_vec3"].return_array_scale = 1
    file.functions["ufbx_sample_geometry_cache_vec3"].return_array_scale = 1

    for func in file.functions.values():
        t = file.types[func.return_type]
        if t.kind == "pointer" and not func.alloc_type:
            func.nullable_return = True

    for mf in member_functions:
        if not mf.member_name:
            mf.member_name = mf.func.replace("ufbx_", "")
        fn = file.functions[mf.func]

        self_index = -1
        for ix, arg in enumerate(fn.arguments):
            t = file.types[arg.type]
            while t.inner:
                t = file.types[t.inner]
            if t.base_name == mf.self_type:
                self_index = ix
        if self_index < 0:
            raise RuntimeError(f"Could not find self ({mf.self_type}) for member {mf.func}")
        mf.self_index = self_index

        file.structs[mf.self_type].member_functions.append(mf.func)

        file.member_functions[mf.func] = mf

    for mg in member_globals:
        file.structs[mg.self_type].member_globals.append(mg.global_name)
        file.member_globals[mg.global_name] = mg

    allow_missing_enum_count = ["ufbx_progress_result"]
    for en in file.enums.values():
        if en.flag: continue
        if en.name in allow_missing_enum_count: continue
        count_name = en.name.upper() + "_COUNT"
        if count_name not in file.constants:
            raise RuntimeError(f"enum {en.name} is missing UFBX_ENUM_TYPE()")

    for arch in archs:
        layout_file(arch, file)

    path_dst = os.path.join(src_path, "build", "ufbx_typed.json")
    with open(path_dst, "wt") as f:
        json.dump(to_json(file), f, indent=2)
