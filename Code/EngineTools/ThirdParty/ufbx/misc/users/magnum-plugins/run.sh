#!/usr/bin/env bash
set -eux

BUILD_PATH=$(dirname -- "$0")
ROOT_PATH=$(realpath "$BUILD_PATH/../../..")

docker run \
    -v "/$ROOT_PATH:/ufbx" \
    ufbx.users.magnum-plugins
