import subprocess
import os

def is_ascii(s):
    try:
        _ = s.encode("ascii")
        return True
    except UnicodeEncodeError:
        return False

if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser("generate_hashes.py --exe <exe> -o hashes.txt")
    parser.add_argument("--verbose", action="store_true", help="Show output")
    parser.add_argument("--exe", required=True, help="hash_scene.c executable")
    parser.add_argument("-o", required=True, help="Output file path")
    argv = parser.parse_args()

    with open(argv.o, "wt") as f:
        for root, dirs, files in os.walk("data"):
            for file in files:
                path = os.path.join(root, file).replace("\\", "/")
                if not is_ascii(path): continue
                if "_fail_" in path: continue
                if "/fuzz/" in path: continue
                if "/obj_fuzz/" in path: continue
                if "/mtl_fuzz/" in path: continue
                if " " in path: continue

                if file.endswith(".fbx"):
                    prev_output = None
                    for frame in range(0, 10):
                        args = [argv.exe, path]

                        if frame >= 0:
                            frame = frame * frame
                            args += ["--frame", str(frame)]

                        output = subprocess.check_output(args)
                        output = output.decode("utf-8").strip()
                        if output == prev_output:
                            break
                        line = f"{output} {frame:3} {path}"
                        if argv.verbose:
                            print(line)
                        print(line, file=f)
                        prev_output = output
                elif file.endswith(".obj") or file.endswith(".mtl"):
                    args = [argv.exe, path]

                    frame = 0
                    output = subprocess.check_output(args)
                    output = output.decode("utf-8").strip()
                    line = f"{output} {frame:3} {path}"
                    if argv.verbose:
                        print(line)
                    print(line, file=f)
                    prev_output = output
