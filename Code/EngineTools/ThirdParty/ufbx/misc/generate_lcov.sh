#!/usr/bin/env bash
set -x
set -e

mkdir -p build

LLVM_COV="${LLVM_COV:-llvm-cov}"
LLVM_GCOV=$(realpath misc/llvm_gcov.sh)
chmod +x misc/llvm_gcov.sh

$LLVM_COV gcov ufbx -b
lcov --directory . --base-directory . --gcov-tool $LLVM_GCOV --config-file misc/lcovrc --capture -o coverage.lcov
