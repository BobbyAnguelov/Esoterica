#!/usr/bin/env bash
set -e
set -x

DATASET_ROOT="$1"

# ASAN dataset
# TODO: --dedicated-allocs when available
clang -O2 -lm -fsanitize=address -fsanitize=leak test/check_fbx.c -o build/heavy_check_fbx_asan
python3 misc/check_dataset.py --root $DATASET_ROOT --exe build/heavy_check_fbx_asan

# FP32 dataset
clang -O2 -lm -DUFBX_REAL_IS_FLOAT test/check_fbx.c -o build/heavy_check_fbx_float
python3 misc/check_dataset.py --root $DATASET_ROOT --exe build/heavy_check_fbx_float
