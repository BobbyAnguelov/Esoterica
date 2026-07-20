import math
import random
import argparse
import transmute_fbx as tfbx

Node = tfbx.Node
Value = tfbx.Value

def max_codepoint(width):
    if width == 0:
        return -1
    elif width == 1:
        return 0x7f
    elif width == 2:
        return 0x7ff
    elif width == 3:
        return 0xffff
    elif width == 4:
        return 0x10_ffff
    else:
        raise ValueError(f"Unsupported width: {width}")

def codepoint_to_utf8(codepoint, width, *, allow_overflow=False):
    """Unrestricted codepoint to UTF-8"""

    if not allow_overflow:
        assert codepoint <= max_codepoint(width)

    c = codepoint
    if width == 1:
        return bytes([c])
    elif width == 2:
        return bytes([
            0b1100_0000 | ((c >> 6) & 0b0001_1111),
            0b1000_0000 | ((c >> 0) & 0b0011_1111),
        ])
    elif width == 3:
        return bytes([
            0b1110_0000 | ((c >> 12) & 0b0000_1111),
            0b1000_0000 | ((c >>  6) & 0b0011_1111),
            0b1000_0000 | ((c >>  0) & 0b0011_1111),
        ])
    elif width == 4:
        return bytes([
            0b1111_0000 | ((c >> 18) & 0b0000_0111),
            0b1000_0000 | ((c >> 12) & 0b0011_1111),
            0b1000_0000 | ((c >>  6) & 0b0011_1111),
            0b1000_0000 | ((c >>  0) & 0b0011_1111),
        ])
    else:
        raise ValueError(f"Unsupported width: {width}")

def int_to_bytes(value):
    num_bytes = int(math.ceil(math.log2(value + 1) / 8))
    return value.to_bytes(num_bytes, "big", signed=False)

def valid_utf8(utf8):
    try:
        utf8.decode("utf-8")
        return True
    except UnicodeDecodeError:
        return False

fuzz_encodings = {
    b"",
    b"\x00",
    b"\xff",
    b"\xff\xff",
    b"\xff\xff\xff",
    b"\xff\xff\xff\xff",
    b"Hello world",
    b"Hello\xffworld",
}

for width in range(1, 4+1):
    for codepoint in range(max_codepoint(width) - 1):
        prev = codepoint_to_utf8(codepoint, width)
        next = codepoint_to_utf8(codepoint + 1, width)
        if valid_utf8(prev) != valid_utf8(next):
            fuzz_encodings.add(prev)
            fuzz_encodings.add(next)

for width in range(1, 4+1):
    fuzz_encodings.add(codepoint_to_utf8(max_codepoint(width - 1) + 1, width))
    fuzz_encodings.add(codepoint_to_utf8(max_codepoint(width), width))

for width in range(1, 4+1):
    for n in range(0x10ffff):
        codepoint = (n*n)//7 + n
        if codepoint > max_codepoint(width):
            break
        fuzz_encodings.add(codepoint_to_utf8(codepoint, width))

for n in range(0x400):
    fuzz_encodings.add(int_to_bytes(n))

for n in range(0, 0x1_00_00, 64):
    fuzz_encodings.add(int_to_bytes(n))

fuzz_encodings.add(codepoint_to_utf8(max_codepoint(4) + 1, 4, allow_overflow=True))
for n in range(32):
    codepoint = 0x10FFFF + n**4
    assert codepoint <= 0x1FFFFF
    fuzz_encodings.add(codepoint_to_utf8(codepoint, 4, allow_overflow=True))
fuzz_encodings.add(codepoint_to_utf8(0x1FFFFF, 4, allow_overflow=True))

random.seed(1)
for n in range(200):
    for k in range(1, 4+1):
        fuzz_encodings.add(bytes(random.choices(range(256), k=k)))

good = []
bad = []
for enc in sorted(fuzz_encodings, key=lambda e: (len(e), e)):
    if valid_utf8(enc):
        good.append(enc)
    else:
        bad.append(enc)

def fmt_fbx_props(encodings, ascii):
    for enc in encodings:
        hex = b"".join(f"{x:02x}".encode("ascii") for x in enc)
        if ascii:
            string = enc.replace(b"\"", b"&quot;")
        else:
            string = enc
        yield Node(b"P", [Value(b"S", hex), Value(b"S", b""), Value(b"S", b""), Value(b"S", b""), Value(b"S", string)], [])

def fmt_fbx_model_name(name, ascii):
    if ascii:
        return Value(b"S", f"Model::{name}".encode("utf-8"))
    else:
        return Value(b"S", f"{name}\x00\x01Model".encode("utf-8"))

def fmt_fbx_root(ascii):
    fbx_root = Node(b"", [], [])

    fbx_objects = Node(b"Objects", [], [])
    fbx_root.children.append(fbx_objects)

    fbx_good = Node(b"Model", [Value(b"L", 1), fmt_fbx_model_name("Good", ascii), Value(b"S", b"Mesh")], [])
    fbx_objects.children.append(fbx_good)

    fbx_good_props = Node(b"Properties70", [], list(fmt_fbx_props(good, ascii)))
    fbx_good.children.append(fbx_good_props)

    fbx_bad = Node(b"Model", [Value(b"L", 2), fmt_fbx_model_name("Bad", ascii), Value(b"S", b"Mesh")], [])
    fbx_objects.children.append(fbx_bad)

    fbx_bad_props = Node(b"Properties70", [], list(fmt_fbx_props(bad, ascii)))
    fbx_bad.children.append(fbx_bad_props)

    ok = [b"\xff" + enc for enc in good]

    fbx_ok = Node(b"Model", [Value(b"L", 3), fmt_fbx_model_name("Ok", ascii), Value(b"S", b"Mesh")], [])
    fbx_objects.children.append(fbx_ok)

    fbx_ok_props = Node(b"Properties70", [], list(fmt_fbx_props(ok, ascii)))
    fbx_ok.children.append(fbx_ok_props)

    return fbx_root

parser = argparse.ArgumentParser("unicode_test_gen.py")
parser.add_argument("outfile", help="Output filename")
argv = parser.parse_args()

root = fmt_fbx_root(ascii=False)
with open(argv.outfile, "wb") as f:
    tfbx.binary_dump_root(f, root, tfbx.BinaryFormat(7500, False), b"")
