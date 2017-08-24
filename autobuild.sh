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


QTVER="5.8"

if test -d /opt/local/bin ; then
	PATH=$PATH:/opt/local/bin ; fi

# Wickr Environment overrides...

if test ! -z "$WICKR_QTDIR" ; then
    QTDIR="$WICKR_QTDIR" ; fi

if test ! -z "$WICKR_QTVER" ; then
    QTVER="$WICKR_QTVER" ; fi

abs=`pwd`
pwd=`basename $abs`
pwd="../$pwd"

# future use get version...
num=`cat $abs/BUILD_NUMBER`
longver="$num"
maj=`expr $num / 1000000`
num=`expr $num - ${maj}000000`
min=`expr $num / 10000`
num=`expr $num - ${min}0000`
pat=`expr $num / 100`
bld=`expr $num % 100`
if test "$bld" -lt "10" ; then
    bld="00$bld"
else
    bld="0$bld"
fi
release=`expr $num - ${pat}00`
version="${maj}.${min}.${pat}"

btype="$2"
product="$1"

case "$btype" in
    beta)
        qtype="CONFIG+=debug CONFIG+=wickr_beta CONFIG+=wickr_compliance_bot CONFIG+=use_wickr_npl"
        bldtype="linux"
        isrelease=false
        build_ext="beta"
        install_ext="Beta"
        svc_build_ext="debug"
        svc_install_ext="Debug"
        ;;
    alpha)
        qtype="CONFIG+=debug CONFIG+=wickr_compliance_bot CONFIG+=use_wickr_npl"
        bldtype="linux"
        isrelease=false
        build_ext="alpha"
        install_ext="Alpha"
        svc_build_ext="debug"
        svc_install_ext="Debug"
        ;;
    release)
        qtype="CONFIG+=wickr_compliance_bot CONFIG+=use_wickr_npl"
        bldtype="linux.release"
        isrelease=true
        build_ext=""
        install_ext=""
        svc_build_ext=""
        svc_install_ext=""
        ;;
esac

case "$product" in
    messenger)
        qtype="$qtype CONFIG+=wickr_messenger"
        ;;
    cloud)
        ;;
esac

if test -z "$QTDIR" ; then
    if test -d /usr/local/wickr/Qt-${QTVER} ; then
        QTDIR=`echo /usr/local/wickr/Qt-${QTVER}`
    else
        QTDIR=`echo ${HOME}/Qt/$QTVER`
    fi
fi
PATH="${QTDIR}/bin:$PATH"
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

build=autobuild-$btype
deploy="$abs/$build/bots.deploy"
output="$abs/autobuild-output/wickrio_$1_$2"

export PATH QTDIR INCLUDE LIB LIBPATH BUILD_CMD
echo "building $type"

mkdir -p $build
rm -rf "$build"/*

set -e
#make
#make update
make $bldtype
make $bldtype.install
(cd $build ; qmake ../wickr-wickrio.pro $qmake $qtype)
(cd $build ; $BUILD_CMD)

# Deploy this thing
rm -rf "$deploy"
mkdir -p "$deploy"
rm -rf "$output"
mkdir -p "$output"

echo "Deploy directory: $deploy"

echo "Create compliance_bot for $product $btype"
build_number=`cat $abs/clients/compliance_bot/BUILD_NUMBER`
binary_dir="$abs/$build"
$abs/clients/compliance_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"

echo "Create welcome_bot for $product $btype"
build_number=`cat $abs/clients/welcome_bot/BUILD_NUMBER`
binary_dir="$abs/$build"
$abs/clients/welcome_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"

echo "going to create $btype for services"
build_number=`cat $abs/services/BUILD_NUMBER`
$abs/services/installer/linux/scripts/deploy64 $binary_dir $build_number "$svc_build_ext" "$svc_install_ext" $isrelease "$deploy"

(cd $deploy ; zip -r "$output/bots-${version}.zip" *.deb *.sha256)

echo "ZIP File: $output/bots-${version}.zip"
