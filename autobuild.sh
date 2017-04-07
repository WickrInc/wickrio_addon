#!/bin/sh

# Things needed to run autobuild...
#
# A Qt install and the QT Version used.  This has some hardcoded defaults
# but can be overriden with WICKR_XXX environment variables.
#
# What to build.  By default this is enterprise & consumer client.  This
# can be overriden with WICKR_CLIENTS
#
# Type of build, passed as argv1.  This includes "release", "debug".
#
# The build is created in ../autobuild-release or ../autobuild-beta.  The
# build directory is cleared by default on each run.


QTVER="5.7"

if test -d /opt/local/bin ; then
	PATH=$PATH:/opt/local/bin ; fi

# Wickr Environment overrides...

if test ! -z "$WICKR_QTDIR" ; then
    QTDIR="$WICKR_QTDIR" ; fi

if test ! -z "$WICKR_QTVER" ; then
    QTVER="$WICKR_QTVER" ; fi

platform=`uname`
abs=`pwd`
pwd=`basename $abs`
pwd="../$pwd"

qtype="CONFIG+=wickr_compliance_bot"

case "$platform" in
Darwin)
    if test -z "$QTDIR" ; then
	if test -d ${HOME}/QtEnterprise ; then
	    QTDIR=`echo ${HOME}/QtEnterprise/$QTVER/clang_64`
	else
	    QTDIR=`echo ${HOME}/Qt/$QTVER/clang_64`
	fi
    fi
    PATH="${QTDIR}/bin:$PATH"
    platform="osx"
    arch="x86_64"
    nproc=`sysctl -n hw.ncpu`
    qmake="-r -spec macx-clang CONFIG+=x86_64"
    BUILD_CMD="make -j$nproc"
    ;;
Linux)
    if test -z "$QTDIR" ; then
	if test -d /usr/local/wickr/Qt-${QTVER} ; then
	    QTDIR=`echo /usr/local/wickr/Qt-${QTVER}`
	else
	    QTDIR=`echo ${HOME}/Qt/$QTVER`
	fi
    fi
    PATH="${QTDIR}/bin:$PATH"
    platform="linux"
    arch=`uname -m`
    nproc=`nproc`
    qmake="-r -spec linux-g++"
    BUILD_CMD="make -j$nproc"
    case "$arch" in
    i386|i486|i568|i686)
	arch="i386"
	gcc="gcc"
	scrarch=""
	debarch="i386"
	generic="generic-32"
	;;
    x86_64|amd64)
	arch="x86_64"
	gcc="gcc_64"
	scrarch="64"
	debarch="amd64"
	generic="generic-64"
	;;
    esac
    ;;
# anything else, maybe windows...hardest to reliably identify
*)
    BUILD_CMD="jom"
    LDFLAGS="-Lc/Qt/$QTVER/msvc2013/lib -L/usr/local/lib"
    CPPFLAGS="-I/c/Qt/$QTVER/msvc2013/include -I/usr/local/include"
    if test -z "$QTDIR" ; then
	if test -d "/c/QtEnterprise" ; then
	    QTDIR=`echo /c/QtEnterprise/$QTVER/msvc2013`
	    TOOLS="/c/QtEnterprise/Tools"
	else
	    QTDIR=`echo /c/Qt/$QTVER/msvc2013`
	    TOOLS="/c/Qt/Tools"
	fi
    fi
    PATH="/c/Qt/$QTVER/msvc2013/bin:/c/Qt/Tools/QtCreator/bin:/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/bin:/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/bin:/c/Qt/$QTVER/msvc2013/bin:/c/Program Files (x86)/Windows Kits/8.1/bin/x64:${PATH}"
    INCLUDE="/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/INCLUDE":"/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/ATLMFC/INCLUDE":"/c/Program Files (x86)/Windows Kits/8.1/include/shared":"/c/Program Files (x86)/Windows Kits/8.1/include/":"/c/Program Files (x86)/Windows Kits/8.1/include/um"
    LIB="/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/LIB:/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/ATLMFC/LIB:/c/Program Files (x86)/Windows Kits/8.1/lib/winv6.3/um/x86"
    LIBPATH="/c/WINDOWS/Microsoft.NET/Framework/v4.0.30319:/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/LIB:/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/ATLMFC/LIB:/c/Program Files (x86)/Windows Kits/8.1/References/CommonConfiguration/Neutral:/c/Program Files (x86)/Microsoft SDKs/Windows/v8.1/ExtensionSDKs/Microsoft.VCLibs/12.0/References/CommonConfiguration/neutral:"
	
    platform="win32"
    arch="i386"
    qmake=" -r -spec win32-msvc2013"
    ;;
esac

btype="release"
build=autobuild-$btype
deploy="$build/compliance.deploy"
output="$abs/autobuild-output/compliance"

export PATH QTDIR INCLUDE LIB LIBPATH BUILD_CMD
echo "building $type for ${platform}..."

mkdir -p $build
rm -rf "$build"/*

case "$platform" in
osx)
    echo "DONE!"
#    set -e
#    (cd $build ; qmake ../wickr-wickrio.pro $qmake $qtype)
#    (cd $build ; $BUILD_CMD)
    ;;
linux)
    set -e
    make
    make update
    make linux.release
    make linux.release.install
    (cd $build ; qmake ../wickr-wickrio.pro $qmake $qtype)
    (cd $build ; $BUILD_CMD)


    # Deploy this thing
    rm -rf "$deploy"
    mkdir -p "$deploy"
    rm -rf "$output"
    mkdir -p "$output"
    $abs/clients/compliance_bot/installers/linux/scripts/create_prod "$deploy"
    $abs/services/installers/linux/scripts/create_prod "$deploy"

    (cd $deploy ; zip -r "$output/compliance-${version}.zip" *)
    ;;
win32)
    echo "DONE!"
#    set -e
#    make
#    make update
#    make win32.release
#    make win32.release.install
#    (cd $build ; qmake ../wickr-wickrio.pro $qmake $qtype)
#    (cd $build ; $BUILD_CMD)
    ;;
esac
