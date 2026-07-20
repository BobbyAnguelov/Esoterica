#!/usr/bin/env python3
import re
import subprocess
import os
import sys

PATH = "build/feature_size"
SECTIONS = [".text", ".rodata", ".data.rel.ro", ".eh_frame", ".rela.dyn"]

RE_FEATURE = re.compile(r"#if.*defined\(UFBX_ENABLE_([A-Z0-9_]+)\).*")
RE_SIZE = re.compile(r"(\.[a-z0-9_.]+)\s+([0-9]+)\s+([0-9]+)")
features = []

with open("ufbx.c", "rt") as f:
    for line in f:
        line = line.strip()
        m = RE_FEATURE.match(line)
        if m:
            features.append(m.group(1))

os.makedirs(PATH, exist_ok=True)

def measure_size(name, defines):
    filename = f"ufbx_{name}.so"
    dst = os.path.join(PATH, filename)
    args = ["clang", *sys.argv[1:], "-fPIC", "-shared"]
    for d in defines:
        args.append(f"-D{d}")
    args += ["ufbx.c", "-o", dst]
    print("$ " + " ".join(args))
    subprocess.check_call(args)

    sections = { }

    args = ["size", "-A", dst]
    print("$ " + " ".join(args))
    result = subprocess.check_output(args, encoding="utf-8", errors="replace")
    for line in result.splitlines():
        line = line.strip()
        m = RE_SIZE.match(line)
        if m:
            section, size, addr = m.groups()
            sections[section] = int(size)

    size = sum(sections.get(s, 0) for s in SECTIONS)
    print(f"  -> {size/1000:.1f}kB")
    return size

base_size = measure_size("minimal", ["UFBX_MINIMAL"])
delta_sizes = []

for feature in features:
    name = feature.lower()
    size = measure_size(name, ["UFBX_MINIMAL", f"UFBX_ENABLE_{feature}"])
    delta_sizes.append((name, size - base_size))

print()

print(f"BASE {base_size/1000:.1f}kB")
for name, size in delta_sizes:
    print(f"{name.upper()} {size/1000:.1f}kB")
