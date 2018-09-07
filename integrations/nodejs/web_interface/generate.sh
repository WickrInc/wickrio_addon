#!/bin/sh

if [ $# -ne 1 ]
  then
    echo "Usage: generate.sh <destination path>"
    exit 1
fi
mkdir -p temp/wickrio_addon
cp package.json.distribution temp/package.json
cp install.sh.distribution temp/install.sh
cp start.sh stop.sh configure.sh upgrade.sh server.js VERSION temp
cp VERSION $1/VERSION
cd ../wickrio_addon
cp -r build/ install.sh ../web_interface/temp/wickrio_addon
cp package.json.distribution ../web_interface/temp/wickrio_addon/package.json
cd ../web_interface/temp
tar czf $1/software.tar.gz *
cd ..
rm -r temp
