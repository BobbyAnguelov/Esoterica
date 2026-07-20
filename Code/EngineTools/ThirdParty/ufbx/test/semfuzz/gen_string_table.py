import os
import common
import glob

def collect_strings(path):
    with open(path, "rt", encoding="utf-8") as f:
        for line in f:
            for m in common.re_str.findall(line):
                yield common.unescape_str(m[1:-1])

def collect_fbx_strings(path):
    with open(path, "rt", encoding="utf-8", errors="ignore") as f:
        for line in f:
            for m in common.re_str.findall(line):
                yield m[1:-1]

strings = {
    "", "P", "R", "S", "T", "X", "Y", "Z", "UV", "OO", "OP", "PO", "PP",
    "Version", "Creator", "Type", "Comment", "Count", "NormalsW",
    "BinormalsW", "TangentsW", "Name",
    "A", "A+", "Property", "Connect",
}
    
for file in ("ufbx.h", "ufbx.c"):
    strings.update(collect_strings(os.path.join(common.repo_path, file)))
strings = sorted(strings)
strings_set = set(strings)

unknown_strings = set()
for file in glob.glob(os.path.join(common.repo_path, "data", "*_ascii.fbx")):
    if "synthetic_texture_split" in file: continue
    for s in collect_fbx_strings(file):
        if not s or s in strings_set or len(s) > 64:
            continue
        unknown_strings.add(s)
unknown_strings = sorted(unknown_strings)

with open(common.string_table_c_path, "wt", encoding="utf-8") as f:
    print("#include \"string_table.h\"", file=f)
    print("", file=f)
    print("const char *string_table[] = {", file=f)
    for s in strings:
        print(f"\t\"{common.escape_str(s)}\",", file=f)
    print("};", file=f)
    print("", file=f)
    print("const char *unknown_string_table[] = {", file=f)
    for s in unknown_strings:
        print(f"\t\"{common.escape_str(s)}\",", file=f)
    print("};", file=f)

with open(common.string_table_h_path, "wt", encoding="utf-8") as f:
    print("#ifndef SEMFUZZ_STRING_TABLE_H", file=f)
    print("#define SEMFUZZ_STRING_TABLE_H", file=f)
    print("", file=f)
    print("#ifdef __cplusplus", file=f)
    print("extern \"C\" {", file=f)
    print("#endif", file=f)
    print("", file=f)
    print(f"#define STRING_TABLE_SIZE {len(strings)}", file=f)
    print("extern const char *string_table[STRING_TABLE_SIZE];", file=f);
    print("", file=f)
    print(f"#define UNKNOWN_STRING_TABLE_SIZE {len(unknown_strings)}", file=f)
    print("extern const char *unknown_string_table[UNKNOWN_STRING_TABLE_SIZE];", file=f);
    print("", file=f)
    print("#ifdef __cplusplus", file=f)
    print("}", file=f)
    print("#endif", file=f)
    print("", file=f)
    print("#endif", file=f)

print(f"Wrote {len(strings)} known strings, {len(unknown_strings)} unknown strings")
