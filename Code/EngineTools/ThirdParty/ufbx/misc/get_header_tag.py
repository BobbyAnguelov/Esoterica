import os
import re
from functools import lru_cache

self_path = os.path.dirname(__file__)
ufbx_path = os.path.join(self_path, "..", "ufbx.h")

@lru_cache()
def get_ufbx_header_version():
    version = None
    with open(ufbx_path, "rt") as f:
        for line in f:
            m = re.match(r"#define\s+UFBX_HEADER_VERSION\s+ufbx_pack_version\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*", line)
            if m:
                version = (int(m.group(1)), int(m.group(2)), int(m.group(3)))
                break

    if not version:
        raise RuntimeError("Could not find version from header")
    return version

if __name__ == "__main__":
    version = get_ufbx_header_version()
    major, minor, patch = version
    print(f"v{major}.{minor}.{patch}")
