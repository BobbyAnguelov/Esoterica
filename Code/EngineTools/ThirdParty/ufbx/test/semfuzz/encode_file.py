import os
import sys
import common
import argparse
import re
import math
import struct
from collections import namedtuple

string_table = list(common.read_string_table(0))
unknown_string_table = list(common.read_string_table(1))
string_to_index = { s: i for i,s in enumerate(string_table) }
unknown_string_to_index = { s: i for i,s in enumerate(unknown_string_table) }

re_name = re.compile(r"^([A-Za-z0-9_]+):\s*(.*)")
re_str = re.compile(r"^\"([^\"]*)\"$")
re_int = re.compile(r"^[-+]?[0-9]+$")
re_word = re.compile(r"^[A-Za-z]+$")
re_float = re.compile(r"[-+]?[0-9]+(?:\.[0-9]+)?(?:[eE][+\-]?[0-9]+)?")
re_magic = re.compile(r"; FBX (\d).(\d).(\d) project file")

Value = namedtuple("Value", "type value")
TYPE_C = 0x0
TYPE_Y = 0x1
TYPE_I = 0x2
TYPE_L = 0x3
TYPE_F = 0x4
TYPE_D = 0x5
TYPE_S = 0x6
TYPE_R = 0x7

type_letters = "CYILFDSR"
type_letters_b = "CYILFDSR".encode("ascii")
arr_letters_b = "cbilfd".encode("ascii")

c_gray = "\033[1;30m"
c_def = "\033[0m"

ARRAY_PATTERN_RANDOM = 0x0
ARRAY_PATTERN_ASCENDING = 0x1
ARRAY_PATTERN_POLYGON = 0x2
ARRAY_PATTERN_REFCOUNT = 0x3

class Field:
    def __init__(self, name, values, is_array):
        self.name = name
        self.values = values
        self.properties = []
        self.fields = []
        self.is_array = is_array
        self.array_min = None
        self.array_max = None
        self.array_type = -1
        self.array_size = 0
        self.array_hash = 0
        self.array_param = 0
        self.array_explicit = False
        self.array_pattern = ARRAY_PATTERN_RANDOM

class File:
    def __init__(self):
        self.version = 0
        self.flags = 0
        self.temp_limit = 0
        self.result_limit = 0
        self.top_field = None

def decode_str(index):
    swap_bit = index & 0x4000
    if swap_bit:
        index ^= 0x4000
    if index < len(string_table):
        result = string_table[index]
    else:
        un_index = (index & 0x7fff) % len(unknown_string_table)
        result = unknown_string_table[un_index]
    if swap_bit and "::" in result:
        type, name = result.split("::", 2)
        result = f"{name}\\0\\1{type}"
    return result

def dump_str(index):
    result = decode_str(index)
    return f"{index:04x}{c_gray}[{result}]{c_def}"

def decode_int(index):
    sign = index >> 15
    exp = (index >> 13) & 0x3
    mantissa = index & 0x1fff
    if exp == 0:
        value = mantissa
    elif exp == 1:
        value = 0x2000 + mantissa * 0x3
    elif exp == 2:
        value = 0x8000 + mantissa * 0x3fc
    elif exp == 3:
        value = 0x800000 + mantissa * 0x3fc00
    return -value if sign else value

def decode_long(index):
    sign = index >> 15
    exp = (index >> 13) & 0x3
    mantissa = index & 0x1fff
    if exp == 0:
        value = mantissa
    elif exp == 1:
        value = 0x2000 + mantissa * 0x3
    elif exp == 2:
        value = 0x8000 + mantissa * 0x40000
    elif exp == 3:
        value = 0x80000000 + mantissa * 0x4000000000000
    return -value if sign else value

def decode_float(index):
    sign = index >> 15
    exp = (((index >> 8) & 0x7f) - 64)
    mantissa = (float(index & 0xff) / 256.0) * 0.5 + 0.5
    value = math.ldexp(mantissa, exp)
    if exp == -64:
        value = 0
    return -value if sign else value

def decode_double(index):
    return decode_float(index)

def decode_word(value):
    return value - 0x10000 if value >= 0x8000 else value

def decode_number(value):
    if value.type == TYPE_C:
        return decode_int(value.value)
    elif value.type == TYPE_Y:
        return decode_word(value.value)
    elif value.type == TYPE_I:
        return decode_int(value.value)
    elif value.type == TYPE_L:
        return decode_long(value.value)
    elif value.type == TYPE_F:
        return decode_float(value.value)
    elif value.type == TYPE_D:
        return decode_double(value.value)
    else:
        assert False

def dump_int(index):
    return f"{index:04x}{c_gray}[{decode_int(index)}]{c_def}"

def dump_long(index):
    return f"{index:04x}{c_gray}[{decode_long(index)}]{c_def}"

def dump_float(index):
    return f"{index:04x}{c_gray}[{decode_float(index)}]{c_def}"

def dump_double(index):
    return f"{index:04x}{c_gray}[{decode_double(index)}]{c_def}"

def dump_word(index):
    return f"{index:04x}{c_gray}[{decode_word(index)}]{c_def}"

def dump_char(index):
    char = chr(index & 0xff)
    return f"{index:04x}{c_gray}[{char}]{c_def}"

def dump_value_raw(value):
    if value.type == TYPE_S:
        return dump_str(value.value)
    elif value.type == TYPE_R:
        return dump_str(value.value)
    elif value.type == TYPE_Y:
        return dump_word(value.value)
    elif value.type == TYPE_I:
        return dump_int(value.value)
    elif value.type == TYPE_L:
        return dump_long(value.value)
    elif value.type == TYPE_F:
        return dump_float(value.value)
    elif value.type == TYPE_D:
        return dump_double(value.value)
    elif value.type == TYPE_C:
        return dump_char(value.value)

def dump_value(value):
    if value.type == TYPE_S:
        return f"{c_gray}[string]{c_def}{dump_str(value.value)}"
    elif value.type == TYPE_R:
        return f"{c_gray}[raw string]{c_def}{dump_str(value.value)}"
    elif value.type == TYPE_Y:
        return f"{c_gray}[word]{c_def}{dump_word(value.value)}"
    elif value.type == TYPE_I:
        return f"{c_gray}[integer]{c_def}{dump_int(value.value)}"
    elif value.type == TYPE_L:
        return f"{c_gray}[long]{c_def}{dump_long(value.value)}"
    elif value.type == TYPE_F:
        return f"{c_gray}[float]{c_def}{dump_float(value.value)}"
    elif value.type == TYPE_D:
        return f"{c_gray}[double]{c_def}{dump_double(value.value)}"
    elif value.type == TYPE_C:
        return f"{c_gray}[char]{c_def}{dump_char(value.value)}"

def dump_field(field, indent=0):
    if field.is_array:
        min_s = dump_value(Value(field.array_type, encode_value(field.array_type, field.array_min)))
        max_s = dump_value(Value(field.array_type, encode_value(field.array_type, field.array_max)))
        tl = type_letters[field.array_type]
        result = f"{'  ' * indent}{dump_str(field.name)}: [array] 2:{c_gray}[{tl}]{c_def} {field.array_size} {field.array_param & 0xffff} {field.array_hash & 0xffff} {min_s} {max_s}"
        return result
    else:
        values = (dump_value(v) for v in field.values)
        result = f"{'  ' * indent}{dump_str(field.name)}: {' '.join(values)}"
        for child in field.fields:
            result += "\n" + dump_field(child, indent + 1)
        return result

def encode_string(content):
    swap_bit = 0
    if isinstance(content, bytes):
        if b"\x00\x01" in content:
            name, type = content.split(b"\x00\x01", maxsplit=1)
            content = b"::".join((type, name))
            swap_bit = 0x4000
        content = content.decode("utf-8", errors="ignore")
    ix = string_to_index.get(content)
    if ix is not None:
        return ix
    else:
        ix = unknown_string_to_index.get(content)
        if not ix:
            ix = hash(content) % len(unknown_string_table)
        return 0x8000 | swap_bit | ix

seen_int_mapping = [{}, {}, {}, {}]

def add_int(exp, value):
    existing = seen_int_mapping[exp].get(value)
    if existing is not None:
        return existing
    index = len(seen_int_mapping[exp])
    seen_int_mapping[exp][value] = index
    return index

def encode_word(value):
    return 0x7fff - value if value < 0 else value

def encode_int(value):
    sign = 1 if value < 0 else 0
    if sign:
        value = -value
    if value < 0x2000:
        # Factor 0x1
        exp = 0
    elif value < 0x8000:
        # Factor 0x3
        exp = 1
        value = add_int(exp, value) & 0x1fff
    elif value < 0x80000000:
        # Factor 0x3fc
        exp = 2
        value = add_int(exp, value) & 0x1fff
    else:
        # Factor 0x3fc00
        exp = 3
        value = add_int(exp, value) & 0x1fff
    return sign << 15 | exp << 13 | value

def encode_long(value):
    sign = 1 if value < 0 else 0
    if sign:
        value = -value
    if value < 0x2000:
        # Factor 0x1
        exp = 0
    elif value < 0x8000:
        # Factor 0x3
        exp = 1
        value = add_int(exp, value) & 0x1fff
    elif value < 0x80000000:
        # Factor 0x40000
        exp = 2
        value = add_int(exp, value) & 0x1fff
    else:
        # Factor 0x4000000000000
        exp = 3
        value = add_int(exp, value) & 0x1fff
    return sign << 15 | exp << 13 | value

def encode_float(value):
    sign = 1 if value < 0 else 0
    if sign:
        value = -value

    if math.isnan(value):
        return sign << 15 | 127 << 8 | 0
    elif not math.isfinite(value):
        return sign << 15 | 127 << 8 | 1

    if value != 0:
        m, exp = math.frexp(value)
    else:
        m, exp = 0.5, -64
    exp = min(max(exp + 64, 0), 126)
    m = min(max(int((m - 0.5) * 2 * 256), 0), 255)
    return sign << 15 | exp << 8 | m

def encode_double(value):
    return encode_float(value)

def encode_integer(value):
    if -2147483648 <= value <= 2147483647:
        return Value(TYPE_I, encode_int(value))
    else:
        return Value(TYPE_L, encode_long(value))

def encode_number(value):
    if isinstance(value, int):
        return encode_integer(value)
    else:
        return Value(TYPE_D, encode_double(value))

def encode_value(type, value):
    if type == TYPE_C:
        return value & 0xff
    if type == TYPE_Y:
        return encode_word(value)
    if type == TYPE_I:
        return encode_int(value)
    if type == TYPE_L:
        return encode_long(value)
    if type == TYPE_F:
        return encode_float(value)
    if type == TYPE_D:
        return encode_double(value)
    if type == TYPE_S:
        return encode_string(value)
    if type == TYPE_R:
        return encode_string(value)

def parse_value(s):
    m = re_str.match(s)
    if m:
        content = m.group(1)
        content = content.replace("&quot;", "\"")
        return Value(TYPE_S, encode_string(content))
    m = re_int.match(s)
    if m:
        value = int(m.group(0))
        return encode_integer(value)
    m = re_word.match(s)
    if m:
        return Value(TYPE_C, ord(s[0]))
    try:
        value = float(s)
        return Value(TYPE_D, encode_double(value))
    except:
        pass
    return None

def parse_ascii(src):
    file = File()
    file.top_field = top_field = Field(0, [], False)
    stack = [top_field]

    def iter_lines():
        buffer = ""
        for line in src:
            line = line.strip()
            if buffer.endswith(",") or buffer.endswith(":") or line.startswith(","):
                buffer += line
            else:
                yield buffer
                buffer = line
        yield buffer

    for line in iter_lines():
        if line == "}":
            field = stack.pop()
            continue

        m = re_magic.match(line)
        if m:
            major, minor, patch = m.groups()
            file.version = int(major) * 1000 + int(minor) * 100 + int(patch) * 10
            continue

        target_array = None
        if stack[-1] and stack[-1].is_array:
            target_array = stack[-1]

        if target_array:
            for v in re_float.findall(line):
                is_int = re_int.match(v)
                value = int(v) if is_int else float(v)
                encoded_value = encode_number(value)
                if target_array.is_array:
                    target_array.values.append(encoded_value)
            continue

        m = re_name.match(line)
        if m:
            name, rest = m.groups()
            opens_scope = False
            if rest.endswith("{"):
                opens_scope = True
                rest = rest[:-1]
            values = [v.strip() for v in rest.split(",")]

            is_array = False
            if len(values) == 1 and values[0].startswith("*"):
                is_array = True

            name_ix = string_to_index.get(name)
            if not name_ix:
                if opens_scope:
                    stack.append(None)
                continue

            values = [parse_value(v) for v in values]
            values = [v for v in values if v]
            field = Field(name_ix, values, is_array)

            if stack[-1]:
                stack[-1].fields.append(field)
            if opens_scope:
                stack.append(field)

    def transform_to_arrays(field):
        if field.is_array or (len(field.values) > 8 and all(v.type in (TYPE_I, TYPE_L, TYPE_F, TYPE_D) for v in field.values)):
            values = [decode_number(v) for v in field.values]
            field.is_array = True
            field.array_type = max(v.type for v in field.values)
            field.array_min = min(values)
            field.array_max = max(values)
            field.array_size = min(len(values), 0xffff)
            field.array_hash = hash(tuple(values))
            name = decode_str(field.name)
            if name == "PolygonVertexIndex":
                field.array_pattern = ARRAY_PATTERN_POLYGON
                field.array_param = sum(1 for v in values if v < 0)
            elif name == "KeyAttrRefCount":
                field.array_pattern = ARRAY_PATTERN_REFCOUNT
                field.array_param = sum(int(v) for v in values)
            elif name == "KeyTime":
                field.array_pattern = ARRAY_PATTERN_ASCENDING
            else:
                field.array_pattern = ARRAY_PATTERN_RANDOM
            field.values = []
        for child in field.fields:
            transform_to_arrays(child)
    transform_to_arrays(file.top_field)

    return file

def parse_binary(f):
    self_path = os.path.dirname(os.path.abspath(__file__))
    misc_path = os.path.realpath(os.path.join(self_path, "..", "..", "misc"))
    sys.path.append(misc_path)
    import transmute_fbx

    fbx_file = transmute_fbx.parse_fbx(f)

    def parse_value(value):
        type_ix = type_letters_b.index(value.type)
        return Value(type_ix, encode_value(type_ix, value.value))

    def parse_node(node):
        name = node.name.decode("ascii")
        name_ix = string_to_index.get(name)
        if not name_ix: return None

        array_values = None
        array_type = -1
        if len(node.values) == 1 and node.values[0].type in b"cbilfd":
            arr = node.values[0]
            array_values = arr.value
            array_type = arr_letters_b.index(arr.type)
        elif len(node.values) > 8 and all(v.type in b"CYILFD" for v in node.values) and name != "Key":
            array_values = [v.value for v in node.values]
            array_type = max(type_letters_b.index(v.type) for v in node.values)
            if array_type == TYPE_Y:
                array_type = TYPE_I

        if array_type >= 0:
            field = Field(name_ix, [], True)
            field.fields = [parse_node(n) for n in node.children]
            field.array_min = min(array_values) if array_values else 0
            field.array_max = max(array_values) if array_values else 0
            field.array_size = min(len(array_values), 0x2000)

            if name == "PolygonVertexIndex":
                field.array_pattern = ARRAY_PATTERN_POLYGON
                field.array_param = sum(1 for v in array_values if v < 0)
            elif name == "KeyAttrRefCount":
                field.array_pattern = ARRAY_PATTERN_REFCOUNT
                field.array_param = sum(int(v) for v in array_values)
            elif name == "KeyTime":
                field.array_pattern = ARRAY_PATTERN_ASCENDING
            else:
                field.array_pattern = ARRAY_PATTERN_RANDOM
            field.array_hash = hash(tuple(array_values))
            field.array_type = array_type
        else:
            values = [parse_value(v) for v in node.values]
            field = Field(name_ix, values, False)
            field.fields = [parse_node(n) for n in node.children]
        field.fields = [f for f in field.fields if f]
        return field

    file = File()
    file.version = fbx_file.version
    if fbx_file.format == "binary-be":
        file.flags |= 0x01000000
    file.top_field = parse_node(fbx_file.root)

    return file

def encode_file(path):
    is_binary = False
    with open(path, "rb") as f:
        magic = f.read(22)
        if magic == b"Kaydara FBX Binary  \x00\x1a":
            is_binary = True

    if is_binary:
        with open(path, "rb") as f:
            return parse_binary(f)
    else:
        with open(path, "rt", encoding="utf-8") as f:
            return parse_ascii(f)

INST_FIELD = 0x0

INST_ARRAY_C = 0x1
INST_ARRAY_B = 0x2
INST_ARRAY_I = 0x3
INST_ARRAY_L = 0x4
INST_ARRAY_F = 0x5
INST_ARRAY_D = 0x6

INST_VALUE_C = 0x7
INST_VALUE_Y = 0x8
INST_VALUE_I = 0x9
INST_VALUE_L = 0xa
INST_VALUE_F = 0xb
INST_VALUE_D = 0xc
INST_VALUE_S = 0xe
INST_VALUE_R = 0xf

def push_instruction(code, inst, level, *words):
    inst_word = level << 4 | inst
    inst_word |= (inst_word ^ 0xff) << 8
    code += struct.pack("<H", inst_word)
    for w in words:
        code += struct.pack("<H", w)

def encode_instructions(code, field, level):
    if field.is_array:
        hash = field.array_hash & 0xffff
        param = field.array_param & 0xffff
        if field.array_explicit:
            push_instruction(code, INST_FIELD, level, field.name)
            for value in field.values:
                push_instruction(code, INST_VALUE_C + value.type, level + 1, value.value)
        else:
            min_v = encode_value(field.array_type, field.array_min)
            max_v = encode_value(field.array_type, field.array_max)
            pat_size = field.array_size | field.array_pattern << 14
            push_instruction(code, INST_ARRAY_C + field.array_type, level, field.name, pat_size, param, hash, min_v, max_v)
    else:
        push_instruction(code, INST_FIELD, level, field.name)
        for value in field.values:
            push_instruction(code, INST_VALUE_C + value.type, level + 1, value.value)
        for child in field.fields:
            encode_instructions(code, child, level + 1)

flag_descs = [
    (0x00000003, 0x00000001, "UFBX_SPACE_CONVERSION_TRANSFORM_ROOT"),
    (0x00000003, 0x00000002, "UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS"),
    (0x00000003, 0x00000003, "UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY"),
    (0x0000000c, 0x00000004, "UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES"),
    (0x0000000c, 0x00000008, "UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY"),
    (0x0000000c, 0x0000000c, "UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK"),
    (0x00000030, 0x00000010, "UFBX_INHERIT_MODE_HANDLING_HELPER_NODES"),
    (0x00000030, 0x00000020, "UFBX_INHERIT_MODE_HANDLING_COMPENSATE"),
    (0x00000030, 0x00000030, "UFBX_INHERIT_MODE_HANDLING_IGNORE"),
    (0x000000c0, 0x00000040, "UFBX_MIRROR_AXIS_X"),
    (0x000000c0, 0x00000080, "UFBX_MIRROR_AXIS_Y"),
    (0x000000c0, 0x000000c0, "UFBX_MIRROR_AXIS_Z"),
    (0x00000100, 0x00000100, "ignore_geometry"),
    (0x00000200, 0x00000200, "ignore_animation"),
    (0x00000400, 0x00000400, "ignore_embedded"),
    (0x00000800, 0x00000800, "disable_quirks"),
    (0x00001000, 0x00001000, "strict"),
    (0x00002000, 0x00002000, "connect_broken_elements"),
    (0x00004000, 0x00004000, "allow_nodes_out_of_root"),
    (0x00008000, 0x00008000, "allow_missing_vertex_position"),
    (0x00010000, 0x00010000, "allow_empty_faces"),
    (0x00020000, 0x00020000, "generate_missing_normals"),
    (0x00040000, 0x00040000, "reverse_winding"),
    (0x00080000, 0x00080000, "normalize_normals"),
    (0x00100000, 0x00100000, "normalize_tangents"),
    (0x00200000, 0x00200000, "retain_dom"),
    (0x00c00000, 0x00400000, "UFBX_INDEX_ERROR_HANDLING_NO_INDEX"),
    (0x00c00000, 0x00800000, "UFBX_INDEX_ERROR_HANDLING_ABORT_LOADING"),
    (0x01000000, 0x01000000, "big_endian"),
    (0x10000000, 0x10000000, "lefthanded"),
    (0x20000000, 0x20000000, "z_up"),
]
flag_values = { name: value for _, value, name in flag_descs }

def disassemble(code):
    n = 0

    print(f"{c_gray}[header]{c_def}")

    version = struct.unpack("<H", code[n:n+2])[0]
    print(f"{version:04x}{c_gray}[version: {version}]{c_def}")
    n += 2

    flags = struct.unpack("<I", code[n:n+4])[0]
    flag_str = ", ".join(name for mask, value, name in flag_descs if (flags & mask) == value)
    if not flag_str:
        flag_str = "0"
    print(f"{flags:08x}{c_gray}[flags: {flag_str}]{c_def}")
    n += 4

    temp_limit = struct.unpack("<I", code[n:n+4])[0]
    print(f"{temp_limit:08x}{c_gray}[temp_limit: {temp_limit}]{c_def}")
    n += 4

    result_limit = struct.unpack("<I", code[n:n+4])[0]
    print(f"{result_limit:08x}{c_gray}[result_limit: {result_limit}]{c_def}")
    n += 4

    print()
    print(f"{c_gray}[bytecode]{c_def}")

    while n < len(code):
        inst_word = struct.unpack("<H", code[n:n+2])[0]
        level = (inst_word >> 4) & 0xf
        inst = inst_word & 0xf
        if inst == INST_FIELD:
            name = struct.unpack("<H", code[n+2:n+4])[0]
            inst_str = f"{inst_word:04x}{c_gray}[{level} INST_FIELD]{c_def}"
            print(f"{inst_str:<38} {dump_str(name)}")
            n += 4
        elif INST_ARRAY_C <= inst <= INST_ARRAY_D:
            name, pat_size, param, hash, min_v, max_v = struct.unpack("<HHHHHH", code[n+2:n+14])
            arr_type = inst - INST_ARRAY_C
            min_s = dump_value_raw(Value(arr_type, min_v))
            max_s = dump_value_raw(Value(arr_type, max_v))
            tl = type_letters[arr_type]
            inst_str = f"{inst_word:04x}{c_gray}[{level} INST_ARRAY {tl}]{c_def}"
            size = pat_size & 0x3fff
            pat = pat_size >> 14
            pattern = ["RANDOM", "ASCENDING", "POLYGON", "REFCOUNT"][pat]
            print(f"{inst_str:<38} {dump_str(name)} {size:04x}{c_gray}[{pattern}, {size}]{c_def} {param:04x}{c_gray}[{param}]{c_def} {hash:04x}{c_gray}[{hash}]{c_def} {min_s} {max_s}")
            n += 14
        elif INST_VALUE_C <= inst <= INST_VALUE_R:
            value = struct.unpack("<H", code[n+2:n+4])[0]
            val_type = inst - INST_VALUE_C
            tl = type_letters[val_type]
            inst_str = f"{inst_word:04x}{c_gray}[{level} INST_VALUE {tl}]{c_def}"
            print(f"{inst_str:<38} {dump_value_raw(Value(val_type, value))}")
            n += 4
        else:
            assert False

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input filename")
    parser.add_argument("-o", type=str, help="Output path")
    parser.add_argument("--out-dir", type=str, help="Output directory")
    parser.add_argument("-f", default=[], action="append", help="Specify flags")
    parser.add_argument("--temp-limit", type=int, help="Temporary allocation limit")
    parser.add_argument("--result-limit", type=int, help="Result allocation limit")
    parser.add_argument("--dump", action="store_true", help="Dump output")
    parser.add_argument("--disassemble", action="store_true", help="Disassemble output")
    argv = parser.parse_args()

    result = encode_file(argv.input)

    for flag in argv.f:
        result.flags |= flag_values[flag]
    result.temp_limit = min(max(argv.temp_limit or 0, 0), 0xffff)
    result.result_limit = min(max(argv.result_limit or 0, 0), 0xffff)

    if argv.dump:
        print(dump_field(result.top_field))
    code = bytearray()
    code += struct.pack("<HIII", result.version, result.flags, result.temp_limit, result.result_limit)
    for field in result.top_field.fields:
        encode_instructions(code, field, 0)
    code = bytes(code)
    if argv.disassemble:
        disassemble(code)
    if argv.o:
        with open(argv.o, "wb") as wf:
            wf.write(code)
    if argv.out_dir:
        name, _ = os.path.splitext(os.path.basename(argv.input))
        path = os.path.join(argv.out_dir, f"{name}.fbb")
        with open(path, "wb") as wf:
            wf.write(code)
