#!/bin/sh

if [ $# -ne 2 ]
  then
    echo "Usage: generate.sh <destination path> <""|"Beta"|"Alpha""
    exit 1
fi
bldtype=$2

mkdir -p temp
cp install.sh.distribution temp/install.sh
cp install.sh start.sh stop.sh configure.sh VERSION temp

sed -e "s/EXTENSION/${bldtype}/g" <start.sh >temp/start.sh

#TODO: Need to copy the actual binary!
cp VERSION $1/VERSION
cd temp
tar czf $1/software.tar.gz *
cd ..
rm -r temp
