#!/bin/sh
if [ $# -ne 2 ]
  then
    echo "Usage: upgrade.sh <OLD_FILEBOT_LOCATION> <NEW_FILEBOT_LOCATION>"
    exit 1
fi
export OLD_FILEBOT_LOCATION=$1
export NEW_FILEBOT_LOCATION=$2
old_version="$OLD_FILEBOT_LOCATION/VERSION"
version=`cat "$old_version"`
cd $OLD_FILEBOT_LOCATION
cp -r files $NEW_FILEBOT_LOCATION
cd ..
mv file_bot file_bot.old_V$version
cd $NEW_FILEBOT_LOCATION/..
mv $NEW_FILEBOT_LOCATION file_bot
