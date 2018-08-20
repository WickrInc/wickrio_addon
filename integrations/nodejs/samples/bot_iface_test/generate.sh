#!/bin/sh

if [ $# -ne 1 ]
  then
    echo "Usage: generate.sh <destination path>"
    exit 1
fi
mkdir -p temp/wickrio_addon
cp package.json.distribution temp/package.json
cp install.sh.distribution temp/install.sh
cp start.sh stop.sh configure.sh bot_iface_test.js VERSION temp
cp VERSION $1/VERSION
cd ../../wickrio_addon
cp -r build/ install.sh ../samples/bot_iface_test/temp/wickrio_addon
cp package.json.distribution ../samples/bot_iface_test/temp/wickrio_addon/package.json
cd ../samples/bot_iface_test/temp
tar czf $1/software.tar.gz *
cd ..
rm -r temp
