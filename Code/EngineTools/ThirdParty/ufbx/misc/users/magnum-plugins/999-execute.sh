#!/usr/bin/env bash
set -eux

cd /work/magnum-plugins

# Copy current UFBX
cp /ufbx/ufbx.{c,h} src/external/ufbx

# Build
cd build
make

# Run tests
ctest -V .
