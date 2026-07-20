#!/usr/bin/env bash

cmd="$1"
shift 1

if [ $cmd == "build-ufbx" ]; then
    afl-clang-fast -static ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_ufbx
elif [ $cmd == "build-ufbx-32" ]; then
    afl-clang-fast -static ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_ufbx_32
elif [ $cmd == "build-ufbx-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DDISCRETE_ALLOCATIONS ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_ufbx_asan
elif [ $cmd == "build-ufbx-asan-32" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DDISCRETE_ALLOCATIONS ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_ufbx_asan_32
elif [ $cmd == "build-cache" ]; then
    afl-clang-fast -static ../../misc/fuzz_cache_persist.c -lm -o fuzz_cache
elif [ $cmd == "build-cache-32" ]; then
    afl-clang-fast -static ../../misc/fuzz_cache_persist.c -lm -o fuzz_cache_32
elif [ $cmd == "build-cache-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DDISCRETE_ALLOCATIONS ../../misc/fuzz_cache_persist.c -lm -o fuzz_cache_asan
elif [ $cmd == "build-cache-asan-32" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DDISCRETE_ALLOCATIONS ../../misc/fuzz_cache_persist.c -lm -o fuzz_cache_asan_32
elif [ $cmd == "build-obj" ]; then
    afl-clang-fast -DLOAD_OBJ -static ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_obj
elif [ $cmd == "build-obj-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DLOAD_OBJ -DDISCRETE_ALLOCATIONS ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_obj_asan
elif [ $cmd == "build-mtl" ]; then
    afl-clang-fast -DLOAD_MTL -static ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_mtl
elif [ $cmd == "build-mtl-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DLOAD_MTL -DDISCRETE_ALLOCATIONS ../../misc/fuzz_ufbx_persist.c -lm -o fuzz_mtl_asan
elif [ $cmd == "build-strtod" ]; then
    afl-clang-fast -static ../../misc/fuzz_strtod_persist.c -lm -o fuzz_strtod
elif [ $cmd == "build-strtod-binary" ]; then
    afl-clang-fast -DBINARY -static ../../misc/fuzz_strtod_persist.c -lm -o fuzz_strtod_binary
elif [ $cmd == "build-strtod-parse" ]; then
    afl-clang-fast -static ../../misc/fuzz_strtod_parse_persist.c -lm -o fuzz_strtod_parse
elif [ $cmd == "build-deflate" ]; then
    afl-clang-fast -static ../../misc/fuzz_deflate_persist.c -lm -lz -o fuzz_deflate
elif [ $cmd == "build-deflate-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast ../../misc/fuzz_deflate_persist.c -lm -lz -o fuzz_deflate_asan
elif [ $cmd == "build-deflate-small-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast -DFUZZ_SMALL ../../misc/fuzz_deflate_persist.c -lm -lz -o fuzz_deflate_small_asan
elif [ $cmd == "build-deflate-roundtrip" ]; then
    afl-clang-fast -static ../../misc/fuzz_deflate_roundtrip.c ../libdeflate/libdeflate.c -lm -o fuzz_deflate_roundtrip
elif [ $cmd == "build-deflate-roundtrip-asan" ]; then
    AFL_USE_ASAN=1 afl-clang-fast ../../misc/fuzz_deflate_roundtrip.c ../libdeflate/libdeflate.c  -lm -lz -o fuzz_deflate_roundtrip_asan
elif [ $cmd == "build-deflate-roundtrip-strict" ]; then
    afl-clang-fast -DSTRICT -static ../../misc/fuzz_deflate_roundtrip.c ../libdeflate/libdeflate.c -lm -o fuzz_deflate_roundtrip_strict
fi

name=$1
shift 1

if [ $cmd == "fuzz" ]; then
    cp -r cases "cases_$name"
    afl-fuzz  "$@" -i "cases_$name" -o "findings_$name" -t 1000 -m 2000 "./$name"
fi
