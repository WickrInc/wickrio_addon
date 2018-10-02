#!/bin/sh
if [ $# -ne 2 ]
  then
    echo "Usage: upgrade.sh <old location> <new location>"
    exit 1
fi
export OLD_LOCATION=$1
export NEW_LOCATION=$2
old_version="$OLD_LOCATION/VERSION"
version=`cat "$old_version"`
cd $OLD_LOCATION
cp welcome_config.ini $NEW_LOCATION
