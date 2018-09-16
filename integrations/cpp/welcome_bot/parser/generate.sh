#!/bin/sh

if [ $# -ne 1 ]
  then
    echo "Usage: generate.sh <destination path>"
    exit 1
fi
mkdir -p temp
cp install.sh.distribution temp/install.sh
cp install.sh start.sh stop.sh configure.sh VERSION temp
#TODO: Need to copy the actual binary!
cp VERSION $1/VERSION
cd temp
tar czf $1/software.tar.gz *
cd ..
rm -r temp
