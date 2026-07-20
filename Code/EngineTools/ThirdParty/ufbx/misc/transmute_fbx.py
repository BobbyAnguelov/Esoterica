#!/usr/bin/env python3
from dataclasses import dataclass
import struct
from typing import Any, Tuple
import zlib
import sys
import io
import argparse

@dataclass
class BinaryFormat:
    version: int
    big_endian: bool
    array_encoding: int = 1
    array_original: bool = False

@dataclass
class Value:
    type: str
    value: Any
    original_data: Tuple[int, bytes] = (0, b"")

@dataclass
class Node:
    name: bytes
    values: list[Value]
    children: list["Node"]

@dataclass
class FbxFile:
    root: Node
    version: int
    format: str
    footer: bytes = b""

def pack(stream, fmt, *args):
    stream.write(struct.pack(fmt, *args))

def unpack(stream, fmt):
    size = struct.calcsize(fmt)
    data = stream.read(size)
    return struct.unpack(fmt, data)

primitive_fmt = {
    b"C": "b", b"B": "b",
    b"Y": "h",
    b"I": "l", b"F": "f",
    b"L": "q", b"D": "d",
}

def binary_parse_value(stream, bf):
    endian = "<>"[bf.big_endian]
    type = stream.read(1)
    fmt = primitive_fmt.get(type)
    if fmt:
        value, = unpack(stream, endian + fmt)
        return Value(type, value)
    if type in b"cbilfd":
        arr_fmt = endian + "L" * 3
        fmt = primitive_fmt[type.upper()]
        count, encoding, encoded_size = unpack(stream, arr_fmt)
        original_data = arr_data = stream.read(encoded_size)
        if encoding == 0: pass # Nop
        elif encoding == 1:
            arr_data = zlib.decompress(arr_data)
        else:
            raise ValueError(f"Unknown encoding: {encoding}")
        values = list(v[0] for v in struct.iter_unpack(endian + fmt, arr_data))
        assert len(values) == count
        return Value(type, values, original_data=(encoding, original_data))
    elif type in b"SR":
        length, = unpack(stream, endian + "L")
        return Value(type, stream.read(length))
    else:
        raise ValueError(f"Bad type: '{type}'")

def binary_parse_node(stream, bf):
    pos = stream.tell()
    endian = "<>"[bf.big_endian]
    head_fmt = endian + "LQ"[bf.version >= 7500] * 3 + "B"
    end_offset, num_values, values_len, name_len = unpack(stream, head_fmt)
    if end_offset == 0 and name_len == 0: return None
    name = stream.read(name_len)
    values_end = stream.tell() + values_len
    values = [binary_parse_value(stream, bf) for _ in range(num_values)]
    children = []
    if stream.tell() != values_end:
        assert stream.tell() < values_end
        stream.seek(pos + values_end)
    while stream.tell() < end_offset:
        node = binary_parse_node(stream, bf)
        if not node: break
        children.append(node)
    return Node(name, values, children)

def parse_fbx(stream):
    magic = stream.read(22)
    if magic == b"Kaydara FBX Binary  \x00\x1a":
        big_endian = stream.read(1) != b"\x00"
        endian = "<>"[big_endian]
        version, = unpack(stream, endian + "L")
        bf = BinaryFormat(version, big_endian)
        children = []
        while True:
            node = binary_parse_node(stream, bf)
            if not node: break
            children.append(node)
        footer = stream.read(16)
        root = Node(b"", [], children)
        format = "binary-be" if big_endian else "binary"
        return FbxFile(root, version, format, footer)
    else:
        # TODO
        raise NotImplementedError()

def binary_dump_value(stream, value: Value, bf: BinaryFormat):
    endian = "<>"[bf.big_endian]
    fmt = primitive_fmt.get(value.type)
    stream.write(value.type)
    if fmt:
        pack(stream, endian + fmt, value.value)
    elif value.type in b"cbilfd":
        fmt = endian + primitive_fmt[value.type.upper()]

        arr_fmt = endian + "L" * 3
        if bf.array_original:
            encoding, arr_data = value.original_data
            pack(stream, arr_fmt, len(value.value), encoding, len(arr_data))
            stream.write(arr_data)
        else:
            with io.BytesIO() as ds:
                for v in value.value:
                    pack(ds, fmt, v)
                arr_data = ds.getvalue()

            count = len(value.value)
            encoding = bf.array_encoding 
            if encoding == 1:
                arr_data = zlib.compress(arr_data)
            encoded_size = len(arr_data)

            pack(stream, arr_fmt, count, encoding, encoded_size)
            stream.write(arr_data)

    elif value.type in b"SR":
        pack(stream, endian + "L", len(value.value))
        stream.write(value.value)
    else:
        raise ValueError(f"Bad type: '{value.type}'")

def binary_dump_node(stream, node: Node, bf: BinaryFormat):
    endian = "<>"[bf.big_endian]
    head_size = 25 if bf.version >= 7500 else 13
    head_null = b"\x00" * head_size
    off_start = stream.tell()
    stream.write(head_null)
    stream.write(node.name)
    off_value_start = stream.tell()
    for value in node.values:
        binary_dump_value(stream, value, bf)
    values_size = stream.tell() - off_value_start
    for child in node.children:
        binary_dump_node(stream, child, bf)
    if node.children or node.name in { b"References", b"AnimationStack", b"AnimationLayer" }:
        stream.write(head_null)
    off_end = stream.tell()
    head_fmt = endian + "LQ"[bf.version >= 7500] * 3 + "B"
    stream.seek(off_start)
    pack(stream, head_fmt, off_end, len(node.values), values_size, len(node.name))
    stream.seek(off_end)

def binary_dump_root(stream, root: Node, bf: BinaryFormat, footer: bytes):
    head_size = 25 if bf.version >= 7500 else 13
    head_null = b"\x00" * head_size
    endian = "<>"[bf.big_endian]

    stream.write(b"Kaydara FBX Binary  \x00\x1a")
    pack(stream, "B", bf.big_endian)
    pack(stream, endian + "L", bf.version)

    for node in root.children:
        binary_dump_node(stream, node, bf)
    stream.write(head_null)

    stream.write(footer)
    stream.write(b"\x00" * 4)

    ofs = stream.tell()
    pad = ((ofs + 15) & ~15) - ofs
    if pad == 0:
        pad = 16

    stream.write(b"\0" * pad)
    pack(stream, endian + "I", bf.version)
    stream.write(b"\0" * 120)
    stream.write(b"\xf8\x5a\x8c\x6a\xde\xf5\xd9\x7e\xec\xe9\x0c\xe3\x75\x8f\x29\x0b")

def ascii_dump_value(stream, value: Value, indent: str):
    if value.type in b"CBYILFD":
        stream.write(str(value.value))
    elif value.type in b"SR":
        s = str(value.value)[2:-1]
        stream.write(f"\"{s}\"")
    elif value.type in b"cbilfd":
        stream.write(f"* {len(value.value)} {{")
        first = True
        for v in value.value:
            stream.write(" " if first else ", ")
            stream.write(str(v))
            first = False
        stream.write(" }")
    else:
        raise ValueError(f"Bad value type: '{value.type}'")

def ascii_dump_node(stream, node: Node, indent: str):
    name = node.name.decode("utf-8")
    stream.write(f"{indent}{name}:")
    first = True
    for value in node.values:
        stream.write(" " if first else ", ")
        first = False
        ascii_dump_value(stream, value, indent + "    ")
    if node.children:
        stream.write(" {\n")
        for node in node.children:
            ascii_dump_node(stream, node, indent + "    ")
        stream.write(indent + "}\n")
    else:
        stream.write("\n")

def ascii_dump_root(stream, root: Node, version: int):
    v0 = version // 1000 % 10
    v1 = version // 100 % 10
    v2 = version // 10 % 10
    stream.write(f"; FBX {v0}.{v1}.{v2} project file\n")
    stream.write("----------------------------------------------------\n")
    for child in root.children:
        ascii_dump_node(stream, child, "")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(usage="transmute_fbx.py src -o dst -v 7400 -f binary-be")
    parser.add_argument("src", help="Source file to read")
    parser.add_argument("--output", "-o", required=True, help="Output filename")
    parser.add_argument("--version", "-v", help="File version")
    parser.add_argument("--format", "-f", help="File format")
    argv = parser.parse_args()

    with open(argv.src, "rb") as f:
        fbx = parse_fbx(f)

    format = argv.format
    if not format:
        format = fbx.format
    version = argv.version
    if not version:
        version = fbx.version
    
    with open(argv.output, "wt" if format == "ascii" else "wb") as f:
        if format == "ascii":
            ascii_dump_root(f, fbx.root, version)
        else:
            if format == "binary-be":
                bf = BinaryFormat(version, True, 0)
            elif format == "binary":
                bf = BinaryFormat(version, False, 1)
            else:
                raise ValueError(f"Unknown format: {format}")
            binary_dump_root(f, fbx.root, bf, fbx.footer)
