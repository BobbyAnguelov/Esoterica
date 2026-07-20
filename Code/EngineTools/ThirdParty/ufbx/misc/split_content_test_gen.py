import transmute_fbx as tfbx
from dataclasses import replace

Node = tfbx.Node
Value = tfbx.Value

divisor = 1

def replace_content(node: Node) -> Node:
    global divisor
    if node.name == b"Content":
        value = node.values[0]
        values = []
        data = value.value
        size = len(data)
        step = max(1, size // divisor)
        for base in range(0, size, step):
            values.append(Value(b"R", data[base:base+step]))
        divisor *= 4
        return replace(node, values=values)
    return replace(node, children=[replace_content(n) for n in node.children])

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(usage="transmute_fbx.py src -o dst -v 7400 -f binary-be")
    parser.add_argument("src", help="Source file to read")
    parser.add_argument("dst", help="Output filename")
    argv = parser.parse_args()

    with open(argv.src, "rb") as f:
        fbx = tfbx.parse_fbx(f)

    fbx = replace(fbx, root=replace_content(fbx.root))

    with open(argv.dst, "wb") as f:
        bf = tfbx.BinaryFormat(fbx.version, False, 1)
        tfbx.binary_dump_root(f, fbx.root, bf, fbx.footer)