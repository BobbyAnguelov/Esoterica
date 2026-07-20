#!/usr/bin/env bash
set -eux

apt-get install -y \
    clang \
    cmake \
    git \
    curl \
    libgl1-mesa-dev

curl https://sh.rustup.rs -sSf > rustup.sh
bash rustup.sh -y

