#!/bin/bash

set -e -x

SCRIPT=$(readlink -f "$0")
SRCDIR=$(dirname "${SCRIPT}")

cd $SRCDIR

build_one() {
    local is64bit=$1
    if [[ $is64bit == "true" ]]; then
        generator="Visual Studio 14 2015 Win64"
        target=seadrive_ext64.dll
    else
        generator="Visual Studio 14 2015"
        target=seadrive_ext.dll
    fi

    rm -f CMakeCache.txt
    cmake -G "$generator" .

    # /nr:false is to prevent msbuild not exiting
    # See https://stackoverflow.com/a/3919906/1467959
    export MSBUILDDISABLENODEREUSE=1
    msbuild -nr:false seadrive_ext.vcxproj

    mkdir -p lib/
    cp Debug/seadrive_ext.dll lib/$target
}

build_one "true"
build_one "false"
