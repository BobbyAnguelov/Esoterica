#!/usr/bin/env bash

set -u
mkdir -p build/lto

CFLAGS="-Wall -Wextra -Werror -Wlto-type-mismatch -Og"

# Compile all objects
printf "Compiling sources\n"
set -e
(set -x; gcc $CFLAGS -c -flto ufbx.c -o build/lto/ufbx-c.o)
(set -x; gcc $CFLAGS -c -flto misc/minimal_load.c -o build/lto/load-c.o)
(set -x; g++ $CFLAGS -c -flto -x c++ ufbx.c -o build/lto/ufbx-cpp.o)
(set -x; g++ $CFLAGS -c -flto -x c++ misc/minimal_load.c -o build/lto/load-cpp.o)
set +e

# Cross link all combinations
RC=0
printf "\nufbx C, load C\n"
(set -x; gcc $CFLAGS -flto build/lto/ufbx-c.o   build/lto/load-c.o   -lm -o build/lto/lto-c-c) || RC=$?
printf "\nufbx C, load C++\n"
(set -x; g++ $CFLAGS -flto build/lto/ufbx-c.o   build/lto/load-cpp.o -lm -o build/lto/lto-c-cpp) || RC=$?
printf "\nufbx C++, load C\n"
(set -x; g++ $CFLAGS -flto build/lto/ufbx-cpp.o build/lto/load-c.o   -lm -o build/lto/lto-cpp-c) || RC=$?
printf "\nufbx C++, load C++\n"
(set -x; g++ $CFLAGS -flto build/lto/ufbx-cpp.o build/lto/load-cpp.o -lm -o build/lto/lto-cpp-cpp) || RC=$?

exit $RC
