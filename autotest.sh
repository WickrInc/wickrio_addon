#!/bin/sh

# Things needed to run autotest...
#
# It is assumed that a build was successful before running this script

if test -d /opt/local/bin ; then
	PATH=$PATH:/opt/local/bin ; fi

# Wickr Environment overrides...

platform=`uname`
abs=`pwd`
pwd=`basename $abs`
pwd="../$pwd"

case "$platform" in
Darwin)
    platform="osx"
    arch="x86_64"
    nproc=`sysctl -n hw.ncpu`
    ;;
Linux)
    platform="linux"
    arch=`uname -m`
    nproc=`nproc`
    ;;
# anything else, maybe windows...hardest to reliably identify
*)
    platform="win32"
    arch="i386"
    ;;
esac

btype="release"
build=autobuild-$btype

echo "testing for ${platform}..."

case "$platform" in
osx)
    echo "DONE!"
    ;;
linux)
    set -e
    (cd $build/testing ; ./maintest)
    ;;
win32)
    echo "DONE!"
    ;;
esac
