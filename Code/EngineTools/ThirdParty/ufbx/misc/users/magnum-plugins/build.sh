#!/usr/bin/env bash
set -eux

BUILD_PATH=$(dirname -- "$0")

docker build \
    --build-arg="CORRADE_SHA=df7a0ba" \
    --build-arg="MAGNUM_SHA=e349336" \
    --build-arg="MAGNUM_PLUGINS_SHA=cfb7d3e4" \
    -t ufbx.users.magnum-plugins \
    $BUILD_PATH
