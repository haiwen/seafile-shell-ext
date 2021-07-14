#!/bin/bash

set -e

unset -v CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

safe_rm() {
    local fn=$1
    if [[ -f "$fn" ]]; then
        rm -rf "$fn"
    fi
}

SCRIPT=$(readlink -f "$0")
SRCDIR=$(dirname "${SCRIPT}")

cd $SRCDIR

safe_rm CMakeCache.txt
safe_rm CMakeFiles
export CXX=g++
cmake -DX32=ON -G "MSYS Makefiles" .
make

safe_rm CMakeCache.txt
safe_rm CMakeFiles
export CXX=x86_64-w64-mingw32-g++
cmake -G "MSYS Makefiles" .
make
