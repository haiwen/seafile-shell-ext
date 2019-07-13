#!/bin/bash

set -e -x

SCRIPT=$(readlink -f "$0")
SRCDIR=$(dirname "${SCRIPT}")

cd $SRCDIR

if ! [[ -f CMakeCache.txt ]]; then
    # TODO: compile both 64bit and 32bit DLL
    cmake -G "Visual Studio 14 2015 Win64" .
fi

# /nr:false is to prevent msbuild not exiting
# See https://stackoverflow.com/a/3919906/1467959
export MSBUILDDISABLENODEREUSE=1
msbuild -nr:false seadrive_ext.vcxproj
