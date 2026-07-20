import argparse
import re

parser = argparse.ArgumentParser(usage="macro_expander.py macro files")
parser.add_argument("macro")
parser.add_argument("files", nargs="*")
argv = parser.parse_args()

macro_lines = []
macro_params = []

RE_NEWLINE = re.compile(r"\n")
RE_DEF = re.compile(f"\n\s*#define\\s+{re.escape(argv.macro)}\s*\\(([^)]*)\\)\s*(.*)\\\\\s*\r?\n", re.M)
RE_LINE = re.compile(r"\t?(.*?)\s*(\\?)\r?\n", re.M)
RE_USE = re.compile(f"{re.escape(argv.macro)}\s*\\(", re.M)

for path in argv.files:
    with open(path, "rt") as f:
        text = f.read()

    lineno = 1
    linestart = 0
    pos = 0
    while True:
        if not macro_lines:
            m = RE_DEF.search(text, pos)
            if m:
                macro_params = [a.strip() for a in m.group(1).split(",")]
                first_line = m.group(2).strip()
                if first_line:
                    macro_lines.append(first_line)
                
                pos = m.end()
                while True:
                    m = RE_LINE.match(text, pos)
                    if not m: break
                    pos = m.end()
                    macro_lines.append(m.group(1))
                    if not m.group(2): break
        else:
            m = RE_USE.search(text, pos)
            if not m: break

            lineno += sum(1 for _ in RE_NEWLINE.finditer(text, linestart, m.start()))
            linestart = m.start()

            tlen = len(text)
            args = [[]]
            level = 0
            pos = m.end()
            while pos < tlen:
                c = text[pos]
                if c == "(":
                    level += 1
                    args[-1].append(c)
                elif c == ")":
                    level -= 1
                    if level < 0: break
                    args[-1].append(c)
                elif c == "," and level == 0:
                    args.append([])
                else:
                    args[-1].append(c)
                pos += 1
            args = ["".join(a).strip() for a in args]

            print()
            print(f"-- {path}:{lineno}")
            for param, arg in zip(macro_params, args):
                print(f"#define {param} {arg}")
            for line in macro_lines:
                print(line)
            for param in macro_params:
                print(f"#undef {param}")

