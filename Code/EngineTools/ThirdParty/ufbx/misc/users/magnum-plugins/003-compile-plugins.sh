#!/usr/bin/env bash
set -eux

cd /work/magnum-plugins

git fetch
git checkout "$MAGNUM_PLUGINS_SHA"

mkdir build
cd build
cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DMAGNUM_WITH_UFBXIMPORTER=ON \
    -DMAGNUM_WITH_STBIMAGEIMPORTER=ON \
    -DMAGNUM_BUILD_TESTS=ON \
    ..
make
