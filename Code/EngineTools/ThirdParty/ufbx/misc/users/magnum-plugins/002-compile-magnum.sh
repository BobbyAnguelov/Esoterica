#!/usr/bin/env bash
set -eux

cd /work/corrade

git fetch
git checkout "$CORRADE_SHA"

mkdir build
cd build
cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    ..
make
make install

cd /work/magnum

git fetch
git checkout "$MAGNUM_SHA"

mkdir build
cd build
cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON \
    ..
make
make install
