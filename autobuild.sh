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


QTVER="5.9"
DEVID="Wickr, LLC"
API_TOKEN="25391301ec2541dfaa1ec0201127e52a"

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
if test "$min" -lt "10" ; then
    min="0$min"
fi
if test "$pat" -lt "10" ; then
    pat="0$pat"
fi
if test "$bld" -lt "10" ; then
    bld="0$bld"
fi
release=`expr $num - ${pat}00`
version="${maj}.${min}.${pat}"
buildout="${version} ${bld}"
versionForDocker="${maj}.${min}.${pat}.${bld}"

echo "release=$release"
echo "version=$version"
echo "versionForDocker=$versionForDocker"

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
    enterprise)
        qtype="$qtype CONFIG+=wickr_enterprise"
        ;;
    cloud)
        ;;
esac

wickrQTDeb="wickr-qt_5.9.4_amd64.deb"

lin_appid=""
case "$product-$btype" in
    cloud-alpha)
        lin_appid="d4fb30fa00bb481ba75f356572890aee"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot=""
        doCoreBot="true"
        consoleDeb="wio_console-debug_${version}-${bld}~debug_amd64.deb"
        integrationDeb="wio_integration-debug_${version}-${bld}.deb"
        complianceDeb=""
        complianceExe=""
        wickrIODockerDeb="wio_docker-alpha_${version}-${bld}~alpha_amd64.deb"
        wickrIOExe="WickrIOSvrDebug"
        wickrIOBotImage="bot-cloud-alpha"
        welcomeDeb=""
        welcomeExe="welcome_botAlpha"
        coreDeb="wio_core_bot-alpha_${version}-${bld}~alpha_amd64.deb"
        coreExe="core_botAlpha"
        coreImage="bot-cloud-core-alpha"
        ;;
    cloud-beta)
        lin_appid="37d0f566718d43148155c9370c06ca12"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot=""
        doCoreBot="true"
        consoleDeb="wio_console-debug_${version}-${bld}~debug_amd64.deb"
        integrationDeb="wio_integration-debug_${version}-${bld}.deb"
        complianceDeb=""
        complianceExe=""
        wickrIODockerDeb="wio_docker-beta_${version}-${bld}~beta_amd64.deb"
        wickrIOExe="WickrIOSvrDebug"
        wickrIOBotImage="bot-cloud-beta"
        welcomeDeb=""
        welcomeExe=""
        coreDeb="wio_core_bot-beta_${version}-${bld}~beta_amd64.deb"
        coreExe="core_botBeta"
        coreImage="bot-cloud-core-beta"
        ;;
    cloud-release)
        lin_appid="f103c55c06af44559256019b1c73412b"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot=""
        doCoreBot="true"
        consoleDeb="wio_console_${version}-${bld}_amd64.deb"
        integrationDeb="wio_integration_${version}-${bld}.deb"
        complianceDeb=""
        complianceExe=""
        wickrIODockerDeb="wio_docker_${version}-${bld}_amd64.deb"
        wickrIOExe="WickrIOSvr"
        wickrIOBotImage="bot-cloud"
        welcomeDeb=""
        welcomeExe=""
        coreDeb="wio_core_bot_${version}-${bld}_amd64.deb"
        coreExe="core_bot"
        coreImage="bot-cloud-core"
        ;;
    messenger-alpha)
        lin_appid="97bd02efd50d4eeca41aa82acac745d6"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot="true"
        doCoreBot="true"
        consoleDeb="wio_console-debug_${version}-${bld}~debug_amd64.deb"
        integrationDeb="wio_integration-debug_${version}-${bld}.deb"
        complianceDeb=""
        complianceExe=""
        wickrIODockerDeb=""
        wickrIOExe=""
        wickrIOBotImage=""
        welcomeDeb="wio_welcome_bot-alpha_${version}-${bld}~alpha_amd64.deb"
        welcomeExe="welcome_botAlpha"
        welcomeImage="bot-messenger-welcome-alpha"
        coreDeb="wio_core_bot-alpha_${version}-${bld}~alpha_amd64.deb"
        coreExe="core_botAlpha"
        coreImage="bot-messenger-core-alpha"
        ;;
    messenger-beta)
        lin_appid="6bf25f5f8b7b4bbfa333d9354e5351ca"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot="true"
        doCoreBot="true"
        consoleDeb="wio_console-debug_${version}-${bld}~debug_amd64.deb"
        integrationDeb="wio_integration-debug_${version}-${bld}.deb"
        complianceDeb=""
        complianceExe=""
        wickrIODockerDeb=""
        wickrIOExe=""
        wickrIOBotImage=""
        welcomeDeb=""
        welcomeExe=""
        welcomeImage=""
        coreDeb="wio_core_bot-beta${version}-${bld}~beta.deb"
        coreExe="core_botBeta"
        coreImage="bot-messenger-core-beta"
        ;;
    messenger-release)
        lin_appid="3affc0dd77b249e492c8cea29441ee60"
        doComplianceBot=""
        doBroadcastBot="true"
        doWelcomeBot="true"
        doCoreBot="true"
        consoleDeb="wio_console_${version}-${bld}_amd64.deb"
        integrationDeb="wio_integration_${version}-${bld}.deb"
        complianceDeb=""
        wickrIODockerDeb=""
        wickrIOExe=""
        wickrIOBotImage=""
        welcomeDeb="wio_welcome_bot_${version}-${bld}_amd64.deb"
        welcomeExe="welcome_bot"
        welcomeImage="bot-messenger-welcome"
        coreDeb="wio_core_bot_${version}-${bld}_amd64.deb"
        coreExe="core_bot"
        coreImage="bot-messenger-core"
        ;;
    enterprise-alpha)
        lin_appid="7723eb32e3434bf6b724c4b04dd35306"
        doComplianceBot="true"
        doBroadcastBot="true"
        doWelcomeBot=""
        doCoreBot=""
        consoleDeb="wio_console-debug_${version}-${bld}~debug_amd64.deb"
        integrationDeb="wio_integration-debug_${version}-${bld}.deb"
        complianceDeb="wio_compliance_bot-alpha_${version}-${bld}~alpha_amd64.deb"
        complianceExe="compliance_provAlpha"
        complianceImage="bot-enterprise-compliance-alpha"
        wickrIODockerDeb="wio_docker-alpha_${version}-${bld}~alpha_amd64.deb"
        wickrIOExe="WickrIOSvrDebug"
        wickrIOBotImage="bot-enterprise-alpha"
        welcomeDeb=""
        welcomeExe=""
        coreDeb=""
        coreExe=""
        ;;
    enterprise-release)
        lin_appid="fe01942f8e394217a6d6e4d594738152"
        doComplianceBot="true"
        doBroadcastBot="true"
        doWelcomeBot=""
        doCoreBot=""
        consoleDeb="wio_console_${version}-${bld}_amd64.deb"
        integrationDeb="wio_integration_${version}-${bld}.deb"
        complianceDeb="wio_compliance_bot_${version}-${bld}_amd64.deb"
        complianceExe="compliance_prov"
        complianceImage="bot-enterprise-compliance"
        wickrIODockerDeb="wio_docker_${version}-${bld}_amd64.deb"
        wickrIOExe="WickrIOSvr"
        wickrIOBotImage="bot-enterprise"
        welcomeDeb=""
        welcomeExe=""
        coreDeb=""
        coreExe=""
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
integrations_output="$output/integrations/software"

export PATH QTDIR INCLUDE LIB LIBPATH BUILD_CMD
echo "building $type"

mkdir -p $build
rm -rf "$build"/*

set -e
make
make update
make $bldtype
make $bldtype.install
(cd $build ; qmake ../wickr-wickrio.pro $qmake $qtype)
(cd $build ; $BUILD_CMD)

#
# Build the node addon
#
(cd integrations/nodejs/wickrio_addon; rm -rf build; npm install; cmake-js rebuild)

#
# Deploy this thing
#
rm -rf "$deploy"
mkdir -p "$deploy"
rm -rf "$output"
mkdir -p "$output"

echo "Deploy directory: $deploy"

#====================================================================================================================================
# Create the Compliance Bot installer
#
if test ! -z "$doComplianceBot" ; then
    echo "Create compliance_bot for $product $btype"
    build_number=`cat $abs/BUILD_NUMBER`
    binary_dir="$abs/$build"
    $abs/clients/compliance_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"
fi

#====================================================================================================================================
# Create the WickrIO Bot installer
#
if test ! -z "$doBroadcastBot" ; then
    echo "Create wickrio_bot for $product $btype"
    build_number=`cat $abs/BUILD_NUMBER`
    binary_dir="$abs/$build"
    $abs/clients/wickrio_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"
fi

#====================================================================================================================================
# Create the Welcome Bot installer
#
if test ! -z "$doWelcomeBot" ; then
    echo "Create welcome_bot for $product $btype"
    build_number=`cat $abs/BUILD_NUMBER`
    binary_dir="$abs/$build"
    $abs/clients/welcome_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"
fi

#====================================================================================================================================
# Create the Core Bot installer
#
if test ! -z "$doCoreBot" ; then
    echo "Create core_bot for $product $btype"
    build_number=`cat $abs/BUILD_NUMBER`
    binary_dir="$abs/$build"
    $abs/clients/core_bot/installers/linux/scripts/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$deploy"
fi

#====================================================================================================================================
# Create the hubot integration software
#
echo "Getting the Hubot integration software from the wickr-integrations submodule"
mkdir -p $output/hubot
(cd $abs/wickr-integrations; ./compress.sh $output/hubot $version)
hubotswdir=$output/hubot
hubotsoftware=$output/hubot/hubot_$version.tar.gz
hubotversion=$output/hubot/VERSION

#
# Copy the hubot software to the integrations software director
#
mkdir -p $integrations_output/hubot
cp $hubotsoftware $integrations_output/hubot/software.tar.gz
cp $hubotversion $integrations_output/hubot

echo "going to create the Samples package"
(cd $abs/integrations/nodejs/installer; ./generate $build_number "$integrations_output" "$deploy")


#====================================================================================================================================
# Create the Docker container image
#
if test ! -z "$wickrIODockerDeb" ; then
    echo "Create docker package for $product $btype"
    build_number=`cat $abs/BUILD_NUMBER`
    binary_dir="$abs/$build"
    $abs/docker/installer/linux/deploy64 $binary_dir $build_number "$build_ext" "$install_ext" $isrelease "$integrations_output" "$deploy"
fi

#====================================================================================================================================
# Create the Integrations Software Package
#
echo "going to create the integration software package"
build_number=`cat $abs/BUILD_NUMBER`
$abs/integrations/installer/linux/scripts/deploy64 $build_number "$svc_build_ext" "$svc_install_ext" $isrelease "$hubotswdir" "$deploy"

#====================================================================================================================================
# Create the Services Software Package
#
echo "going to create $btype for services"
build_number=`cat $abs/BUILD_NUMBER`
$abs/services/installer/linux/scripts/deploy64 $binary_dir $build_number "$svc_build_ext" "$svc_install_ext" $isrelease "$deploy"

#====================================================================================================================================
# Create the Console Software Package
#
echo "going to create $btype for console command package (for Docker)"
build_number=`cat $abs/BUILD_NUMBER`
$abs/services/installer/linux/scripts/consoleCmd_deploy64 $binary_dir $build_number "$svc_build_ext" "$svc_install_ext" $isrelease "$deploy"

#====================================================================================================================================
# Create the Wickr 64 Software Package
#
echo "going to create Qt library package"
$abs/platforms/linux/debian/wickrqt/deploy64 "$deploy"

#====================================================================================================================================
# Create the package for deployment
#
(cd $deploy ; zip -r "$output/bots-${version}.zip" *.deb *.sha256 *.tar.gz)

echo "ZIP File: $output/bots-${version}.zip"

APP_OUT="$output/bots-${version}.zip"
SYM_OUT=""
set -e
sed -e 's/\*/-/g' -e 's/[\"'\'']//g' release.txt > rel2.txt
relnotes=`cat rel2.txt`
rm rel2.txt
set +e

if test ! -z "$lin_appid" ; then
    APP_ID="$lin_appid"

    hockeyid=`curl \
      -F "bundle_short_version=$version" \
      -F "bundle_version=$buildout" \
      -H "X-HockeyAppToken: $API_TOKEN" \
      https://rink.hockeyapp.net/api/2/apps/$APP_ID/app_versions/new \
      | sed s/.*\"id\":// | sed s/,.*//`

    echo hockeyid=$hockeyid

    if [  "$hockeyid" -eq "$hockeyid" ] 2> /dev/null ; then
        if test ! -f "$SYM_OUT" ; then
            curl \
              -X PUT \
              -F "ipa=@$APP_OUT" \
              -F "status=1" \
              -F "notify=0" \
              -F "notes=$relnotes" \
              -F "notes_type=0" \
              -H "X-HockeyAppToken: $API_TOKEN" \
              https://rink.hockeyapp.net/api/2/apps/$APP_ID/app_versions/$hockeyid ;
        else
            curl \
              -X PUT \
              -F "ipa=@$APP_OUT" \
              -F "dsym=@$SYM_OUT" \
              -F "status=1" \
              -F "notify=0" \
              -F "notes=$relnotes" \
              -F "notes_type=0" \
              -H "X-HockeyAppToken: $API_TOKEN" \
              https://rink.hockeyapp.net/api/2/apps/$APP_ID/app_versions/$hockeyid ;
        fi
    else
        echo "call to add version to hockey seemed to fail!";
        exit 1
    fi
fi

#
# Handle the generation of Docker containers
#
mkdir -p docker/packages
cp ${deploy}/${wickrQTDeb} docker/packages
cp ${deploy}/${consoleDeb} docker/packages

if test ! -z "$wickrIODockerDeb" ; then
    cp ${deploy}/${wickrIODockerDeb} docker/packages
fi

if test ! -z "$integrationDeb" ; then
    cp ${deploy}/${integrationDeb} docker/packages
fi

if test ! -z "$coreDeb" ; then
    cp ${deploy}/${coreDeb} docker/packages
fi

if test ! -z "$complianceDeb" ; then
    cp ${deploy}/${complianceDeb} docker/packages
    (cd docker; ./dockerSetup "${wickrQTDeb}" "${consoleDeb}" "${complianceDeb}" "${complianceExe}" "${complianceImage}" "${versionForDocker}" "${integrationDeb}")
fi

if test ! -z "$wickrIODockerDeb" ; then
    cp ${deploy}/${wickrIODockerDeb} docker/packages
    (cd docker; ./dockerSetup "${wickrQTDeb}" "${wickrIODockerDeb}" "${wickrIOExe}" "${wickrIOBotImage}" "${versionForDocker}")
fi

if test ! -z "$welcomeDeb" ; then
    cp ${deploy}/${welcomeDeb} docker/packages
    (cd docker; ./dockerSetup "${wickrQTDeb}" "${consoleDeb}" "${welcomeDeb}" "${welcomeExe}" "${welcomeImage}" "${versionForDocker}" "${integrationDeb}")
fi

rm -rf docker/packages
