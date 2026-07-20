#!/usr/bin/env python3
import argparse
import os
import time
import subprocess
import glob
import re

parser = argparse.ArgumentParser(usage="execute_per_fbx.py --exe loader --root .")
parser.add_argument("--exe", help="Executable to run")
parser.add_argument("--root", default=".", help="Root path to search from")
parser.add_argument("--start", default="", help="Top-level file to start from")
parser.add_argument("--list", default="", help="List of files to use instead of root")
parser.add_argument("--glob", default="", help="Glob of files to use instead of root")
parser.add_argument("--verbose", action="store_true", help="Verbose information")
parser.add_argument("--allow-fail", action="store_true", help="Verbose information")
parser.add_argument("--exclude", default=[], action="append", help="Patterns for excluding files")
parser.add_argument("--exclude-list", help="Name of a file with a list of excluded files")
parser.add_argument("--cycles", default=1, type=int, help="Number of cycles to load the data")
parser.add_argument("--allow-non-fbx", action="store_true", help="Allow non-FBX files")
parser.add_argument("--stdout", help="Redirect standard output")
parser.add_argument("--stderr", help="Redirect standard input")
parser.add_argument("-p", action="append", help="Run multiple permutations, use with #p")
parser.add_argument('remainder', nargs="...")
argv = parser.parse_args()

begin = time.time()

num_tested = 0
num_fail = 0
num_exclude = 0
total_size = 0

exclude_list = set()
if argv.exclude_list:
    with open(argv.exclude_list, "rt") as f:
        exclude_list = set(l.lower().strip() for l in f if l.strip())

def gather_files():
    if argv.list:
        with open(argv.list, "rt", encoding="utf-8") as f:
            for line in f:
                line = line.rstrip()
                if not line:
                    continue
                path = os.path.join(argv.root, line)
                yield path
    elif argv.glob:
        for file in glob.glob(argv.glob):
            yield file
    else:
        for root, dirs, files in os.walk(argv.root):
            for file in files:
                path = os.path.join(root, file)
                yield path

for cycle in range(argv.cycles):
    for path in gather_files():
        file = os.path.basename(path)

        if not argv.allow_non_fbx:
            if not file.lower().endswith(".fbx"): continue
        if file.lower().endswith(".ufbx-fail.fbx"):
            num_fail += 1
            continue

        exclude = file.lower() in exclude_list
        for pattern in argv.exclude:
            if pattern in file:
                exclude = True

        if exclude:
            num_exclude += 1
            continue

        size = os.stat(path).st_size
        display = os.path.relpath(path, argv.root) if not argv.list else path

        if argv.start and display < argv.start:
            continue

        print(f"-- {display}", flush=True)
        total_size += size

        ps = [""]
        if argv.p:
            ps = argv.p

        name = re.sub(r"[^A-Za-z0-9/]+", "_", path)
        name = name.replace("/", "-")

        def filter_arg(r, p):
            if r is None:
                return None
            if r == "#":
                return path.encode("utf-8")
            r = r.replace("#p", p)
            r = r.replace("#n", name)
            return r.encode("utf-8")

        for p in ps:
            if argv.exe:
                rest = argv.remainder[1:]
                if "#" not in rest:
                    rest = [path] + rest
                rest = [filter_arg(r, p) for r in rest]
                args = [argv.exe] + rest

                stdout_path = filter_arg(argv.stdout, p)
                stderr_path = filter_arg(argv.stderr, p)

                stdout = open(stdout_path, "w") if stdout_path else None
                stderr = open(stderr_path, "w") if stderr_path else None

                if argv.verbose:
                    cmdline = subprocess.list2cmdline(args)
                    print(f"$ {cmdline}")

                if argv.allow_fail:
                    try:
                        subprocess.check_call(args, stdout=stdout, stderr=stderr)
                    except Exception as e:
                        print("Failed to load")
                else:
                    subprocess.check_call(args, stdout=stdout, stderr=stderr)

                if stdout:
                    stdout.close()
                if stderr:
                    stderr.close()
        num_tested += 1

end = time.time()
dur = end - begin
print()
print("Success!")
print(f"Loaded {num_tested} files in {int(dur//60)}min {int(dur%60)}s.")
print(f"Processed {total_size/1e9:.2f}GB at {total_size/1e6/dur:.2f}MB/s.")
print(f"Ignored {num_fail} invalid files, {num_exclude} excluded files.")
