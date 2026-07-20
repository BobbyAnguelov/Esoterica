#!/usr/bin/env bash
set -eux

BUILD_PATH=$(dirname -- "$0")

docker build \
    -t ufbx.users.base \
    $BUILD_PATH
