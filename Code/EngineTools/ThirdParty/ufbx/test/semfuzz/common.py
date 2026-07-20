import os
import re

root_path = os.path.dirname(os.path.realpath(__file__))
repo_path = os.path.join(root_path, "..", "..")

esc_tab = {
    "n": "\n",
    "r": "\r",
    "t": "\t",
    "v": "\v",
    "\\": "\\",
    "\"": "\"",
    "0": "\0",
}
esc_tab_rev = { v: k for k,v in esc_tab.items() }
re_str = re.compile(r"\"(?:[^\"]|\\.)*\"")
re_esc = re.compile(r"\\([^x]|x[0-9a-fA-F]{2})")

def unescape_str(s):
    def sub_esc(m):
        e = m.group(1)
        if e[0] == "x":
            return chr(int(e[1:], base=16))
        return esc_tab[e]
    return re_esc.sub(sub_esc, s)

def escape_str(s):
    def inner():
        for c in s:
            if 32 <= ord(c) <= 126 and c not in "\\\"":
                yield c
            elif c in esc_tab_rev:
                yield f"\\{esc_tab_rev[c]}"
            else:
                # yield f"\\x{ord(c):02x}"
                yield "" # TODO
    return "".join(inner())

string_table_c_path = os.path.join(root_path, "string_table.c")
string_table_h_path = os.path.join(root_path, "string_table.h")

def read_string_table(index):
    table_index = -1
    with open(string_table_c_path, "rt", encoding="utf-8") as f:
        for line in f:
            if line.strip() == "const char *string_table[] = {":
                table_index = 0
                continue
            if line.strip() == "const char *unknown_string_table[] = {":
                table_index = 1
                continue
            if table_index == index:
                m = re_str.search(line)
                if m:
                    yield unescape_str(m.group(0)[1:-1])
