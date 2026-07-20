# ufbx [![CI](https://github.com/bqqbarbhg/ufbx/actions/workflows/ci.yml/badge.svg)](https://github.com/bqqbarbhg/ufbx/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/ufbx/ufbx/branch/master/graph/badge.svg)](https://codecov.io/gh/ufbx/ufbx)

Single source file FBX file loader.

```cpp
ufbx_load_opts opts = { 0 }; // Optional, pass NULL for defaults
ufbx_error error; // Optional, pass NULL if you don't care about errors
ufbx_scene *scene = ufbx_load_file("thing.fbx", &opts, &error);
if (!scene) {
    fprintf(stderr, "Failed to load: %s\n", error.description.data);
    exit(1);
}

// Use and inspect `scene`, it's just plain data!

// Let's just list all objects within the scene for example:
for (size_t i = 0; i < scene->nodes.count; i++) {
    ufbx_node *node = scene->nodes.data[i];
    if (node->is_root) continue;

    printf("Object: %s\n", node->name.data);
    if (node->mesh) {
        printf("-> mesh with %zu faces\n", node->mesh->faces.count);
    }
}

ufbx_free_scene(scene);
```

## Documentation

[Online documentation](https://ufbx.github.io/)

## Setup

Copy `ufbx.h` and `ufbx.c` to your project, `ufbx.c` needs to be compiled as
C99/C++11 or more recent. You can also add `misc/ufbx.natvis` to get debug
formatting for the types.

## Features

The goal is to be at feature parity with the official FBX SDK.

* Supports binary and ASCII FBX files starting from version 3000
* Safe
  * Invalid files and out-of-memory conditions are handled gracefully
  * Loaded scenes are sanitized by default, no out-of-bounds indices or non-UTF-8 strings
  * Extensively [tested](#testing)
* Various object types
  * Meshes, skinning, blend shapes
  * Lights and cameras
  * Embedded textures
  * NURBS curves and surfaces
  * Geometry caches
  * LOD groups
  * Display/selection sets
  * Rigging constraints
* Unified PBR material from known vendor-specific materials
* Various utilities for evaluating the scene (can be compiled out if not needed)
  * Polygon triangulation
  * Index generation
  * Animation curve evaluation / layer blending
  * CPU skinning evaluation
  * Subdivision surface evaluation
  * NURBS curve/surface tessellation
* Progress reporting and cancellation
* Support for Wavefront `.obj` files as well

## Platforms

The library is written in portable C (also compiles as C++) and should work on
almost any platform without modification. If compiled as pre-C11/C++11 on an
unknown compiler (not MSVC/Clang/GCC/TCC), some functions will not be
thread-safe as C99 does not have support for atomics.

The following platforms are tested on CI and produce bit-exact results:

* Windows: MSVC x64/x86, Clang x64/x86, GCC MinGW x64
* macOS: Clang x64, GCC x64
* Linux: Clang x64/x86/ARM64/ARM32/PowerPC, GCC x64/x86/ARM64/ARM32, TCC x64/x86
* WASI: Clang WASM

## Testing

* Internal tests run on all platforms listed above
  * 592 test cases / 604 FBX files
* Fuzzed in multiple layers
  * Parsers (fbx binary/fbx ascii/deflate/xml/mcx/obj/mtl) fuzzed using AFL
  * Structured FBX binary/ascii fuzzing using AFL
  * Built-in fuzzing for byte modifications/truncation/out-of-memory
  * Semantic fuzzing for binary FBX and OBJ files
* Public dataset: 4.7GB / 323 files
  * Loaded, validated, and compared against reference .obj files
* Private dataset: 33.6GB / 12618 files
  * Loaded and validated
* Static analysis for maximum stack depth on Linux GCC/Clang
* In total 95% branch line coverage (99% partial line coverage)

## Versioning

The latest commit in the [`master`](https://github.com/ufbx/ufbx/tree/master)
branch contains the latest stable version of the library.

Older versions are tagged as `vX.Y.Z`, patch updates (`Z`) are ABI compatible
and work with older versions of the header from the same minor version (`Y`).
Minor versions within a major verision (`X`) are expected to be source
compatible after `1.0.0` but the `0.Y.Z` releases can break for every minor
release.

## License

```
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Samuli Raivio
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------
```
