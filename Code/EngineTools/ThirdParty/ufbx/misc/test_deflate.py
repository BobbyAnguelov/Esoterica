import os
import sys
import zlib
import subprocess

if len(sys.argv) < 2:
    filepath = os.path.dirname(sys.argv[0])
    if os.name == 'nt':
        exe = os.path.join(filepath, "../build/test_deflate.exe")
    else:
        exe = os.path.join(filepath, "../build/test_deflate")
    exe = os.path.relpath(exe)
else:
    exe = sys.argv[1]

path = "."
if len(sys.argv) >= 3:
    path = sys.argv[2]

def test(data):
    for level in range(1, 10):
        if level < 9:
            print("{},".format(level), end="")
        else:
            print("{}".format(level), end="")
        compressed = zlib.compress(data, level)
        args = [exe, str(len(compressed)), str(len(data))]
        result = subprocess.check_output(args, input=compressed)
        if result != data:
            raise ValueError("Decompression mismatch")

for r,dirs,files in os.walk(path):
    # HACK: Ignore .git directories
    if ".git" in r:
        continue
    
    for file in files:
        sys.stdout.flush()
        path = os.path.join(r, file)
        try:
            print(path, end=': ')
        except:
            # Print fails sometimes with weird filenames (?)
            continue
        try:
            f = open(path, "rb")
        except:
            print("SKIP")
            continue
        b = f.read()
        try:
            test(b)
            print()
        except Exception as e:
            print()
            print("FAIL ({})".format(e))
            sys.exit(1)
        f.close()
