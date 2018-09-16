#!/bin/sh

#
# If the input argument exists then check if it is a file
# if so it should contain a list of key=value entries.
#
if [ -n "$1" ]; then
  if [ -f "$1" ]; then
    . "$1"
  fi
fi

while [ -z "$clientname" ]
do
  echo "prompt: Please enter your client bot's username:"
  read clientname
  if [ -z "$clientname" ]; then
    echo "Cannot leave client bot's username empty!"
  fi
done

echo "Configuration of message text:"

while [ -z "$welcomeusermessage" ]
do
  echo "prompt: Please enter Welcome User Message:"
  read welcomeusermessage
  if [ -z "$welcomeusermessage" ]; then
    echo "Cannot leave Welcome User Message empty!"
  fi
done

while [ -z "$welcomeadminmessage" ]
do
  echo "prompt: Please enter Welcome Admin Message:"
  read welcomeadminmessage
  if [ -z "$welcomeadminmessage" ]; then
    echo "Cannot leave Welcome Admin Message empty!"
  fi
done

while [ -z "$newdevicemessage" ]
do
  echo "prompt: Please enter New Device Message:"
  read newdevicemessage
  if [ -z "$newdevicemessage" ]; then
    echo "Cannot leave New Device Message empty!"
  fi
done

while [ -z "$forgotpasswordmessage" ]
do
  echo "prompt: Please enter Forgot Password Message:"
  read forgotpasswordmessage
  if [ -z "$forgotpasswordmessage" ]; then
    echo "Cannot leave Forgot Password Message empty!"
  fi
done

echo "Configuration of RabbitMQ settings:"

while [ -z "$rabbithost" ]
do
  echo "prompt: Please enter host:"
  read rabbithost
  if [ -z "$rabbithost" ]; then
    echo "Cannot leave rabbit host value empty!"
  fi
done

while [ -z "$rabbitport" ]
do
  echo "prompt: Please enter port:"
  read rabbitport
  if [ -z "$rabbitport" ]; then
    echo "Cannot leave rabbit port value empty!"
  fi
done

while [ -z "$rabbituser" ]
do
  echo "prompt: Please enter user:"
  read rabbituser
  if [ -z "$rabbituser" ]; then
    echo "Cannot leave rabbit user value empty!"
  fi
done

while [ -z "$rabbitpassword" ]
do
  echo "prompt: Please enter password:"
  read rabbitpassword
  if [ -z "$rabbitpassword" ]; then
    echo "Cannot leave rabbit password value empty!"
  fi
done

while [ -z "$rabbitexchange" ]
do
  echo "prompt: Please enter exchange:"
  read rabbitexchange
  if [ -z "$rabbitexchange" ]; then
    echo "Cannot leave rabbit exchange value empty!"
  fi
done

while [ -z "$rabbitqueue" ]
do
  echo "prompt: Please enter queue:"
  read rabbitqueue
  if [ -z "$rabbitqueue" ]; then
    echo "Cannot leave rabbit queue value empty!"
  fi
done

while [ -z "$rabbitvirtualhost" ]
do
  echo "prompt: Please enter virtual host:"
  read rabbitvirtualhost
  if [ -z "$rabbitvirtualhost" ]; then
    echo "Cannot leave rabbit virtual host value empty!"
  fi
done

#
# Outout to the settings file
#
echo "[basicsettings]" > welcome_config.ini
echo "clientname=\"${clientname}\"" >> welcome_config.ini
echo "welcomeUserMessage=\"${welcomeusermessage}\"" >> welcome_config.ini
echo "welcomeAdminMessage=\"${welcomeadminmessage}\"" >> welcome_config.ini
echo "newDeviceMessage=\"${newdevicemessage}\"" >> welcome_config.ini
echo "forgotPasswordMessage=\"${forgotpasswordmessage}\"" >> welcome_config.ini
echo "\n" >> welcome_config.ini

echo "[rabbitqueue]" >> welcome_config.ini
echo "host=\"${rabbithost}\"" >> welcome_config.ini
echo "port=${rabbitport}" >> welcome_config.ini
echo "user=\"${rabbituser}\"" >> welcome_config.ini
echo "password=\"${rabbitpassword}\"" >> welcome_config.ini
echo "exchangename=\"${rabbitexchange}\"" >> welcome_config.ini
echo "queuename=\"${rabbitqueue}\"" >> welcome_config.ini
echo "virtualhost=\"${rabbitvirtualhost}\"" >> welcome_config.ini
echo "\n" >> welcome_config.ini

