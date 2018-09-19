#!/bin/sh

if [ $# -ne 4 ]
  then
    echo "Usage: generate.sh <binary path> <destination path> <""|"Beta"|"Alpha"> </usr/lib rpath>"
    exit 1
fi
binary_path=$1
dest_path=$2
bldtype=$3
usrLibRpath=$4
binary_name=$1/welcome_parser${bldtype}
ipc_name=$1/../ipc/welcome_ipc

echo "Generating welcome_parser${bldtype} integration package"
echo "binary_path=${binary_path}"
echo "dest_path=${dest_path}"
echo "bldtype=${bldtype}"
echo "usrLibRpath=${usrLibRpath}"
echo "binary_name=${binary_name}"

mkdir -p temp
cp ${binary_name} temp
cp ${ipc_name} temp
cp install.sh start.sh stop.sh configure.sh upgrade.sh VERSION temp

#
# Need to change the rpath so that it works on the docker installation
#
chrpath -r "/usr/local/wickr/Qt-5.10/lib:/usr/lib/${usrLibRpath}" temp/welcome_parser${bldtype}
chrpath -r "/usr/local/wickr/Qt-5.10/lib:/usr/lib/${usrLibRpath}" temp/welcome_ipc

sed -e "s/EXTENSION/${bldtype}/g" <start.sh >temp/start.sh

#TODO: Need to copy the actual binary!
cp VERSION ${dest_path}/VERSION
cd temp
tar czf ${dest_path}/software.tar.gz *
cd ..
rm -r temp
