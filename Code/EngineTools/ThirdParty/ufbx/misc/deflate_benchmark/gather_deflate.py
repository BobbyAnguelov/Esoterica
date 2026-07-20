import argparse
import os
import subprocess

parser = argparse.ArgumentParser(description="Gather DEFLATE compressed streams from .fbx files")
parser.add_argument("--exe", help="Executable path for per-file gathering, see `gather_deflate_main.cpp`")
parser.add_argument("-o", help="Output file")
parser.add_argument("--root", help="Root path to look for .fbx files")
argv = parser.parse_args()

data = bytearray()

for root,_,files in os.walk(argv.root):
    for file in files:
        if not file.endswith(".fbx"): continue
        path = os.path.join(root, file)
        print(path)
        data += subprocess.check_output([argv.exe, path])

with open(argv.o, "wb") as f:
    f.write(data)
