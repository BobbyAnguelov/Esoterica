import os
import re
import argparse

self_path = os.path.dirname(os.path.abspath(__file__))
fuzz_path = os.path.join(self_path, "..", "data", "fuzz")

p = argparse.ArgumentParser(prog="add_fuzz_cases.py")
p.add_argument("path", default=fuzz_path, help="Path where to rename files in")
p.add_argument("--ext", default="fbx", help="File extension")
argv = p.parse_args()

fuzz_path = argv.path

fuzz_files = { }
file_queue = []

RE_FUZZ = re.compile(f"fuzz_(\\d+).{re.escape(argv.ext)}")

for name in os.listdir(fuzz_path):
    path = os.path.join(fuzz_path, name)
    with open(path, 'rb') as f:
        content = f.read()
    m = RE_FUZZ.match(name)
    if m:
        fuzz_files[content] = name
    else:
        file_queue.append((name, content))

for name, content in file_queue:
    existing = fuzz_files.get(content)
    if existing:
        print("{}: Exists as {}".format(name, existing))
    else:
        new_name = "fuzz_{:04}.{}".format(len(fuzz_files), argv.ext)
        print("{}: Renaming to {}".format(name, new_name))
        fuzz_files[content] = new_name
        path = os.path.join(fuzz_path, name)
        new_path = os.path.join(fuzz_path, new_name)
        os.rename(path, new_path)
