#!/bin/bash

set -e

unset -v CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

SCRIPT=$(readlink -f "$0")
SRCDIR=$(dirname "${SCRIPT}")

cd $SRCDIR

rm -rf CMakeCache.txt CMakeFiles
export CXX=g++
cmake -G "MSYS Makefiles" .
make

