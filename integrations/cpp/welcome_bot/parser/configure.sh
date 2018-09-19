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

#
# if the ini file already exists, then values are being modified
# read in the values from the ini file
#
if [ -f welcome_config.ini ]; then
  . ./welcome_config.ini 2>/dev/null
#  sed "s/;/#/g" welcome_config.ini | source /dev/stdin # 2>/dev/null
fi

#
# Get the client name if not configured
#
while [ -z "$CLIENT_NAME" ]
do
  echo "prompt:Please enter your client bot's username"
  read CLIENT_NAME
  if [ -z "$CLIENT_NAME" ]; then
    echo "Cannot leave client bot's username empty!"
  fi
done

echo "Configuration of message text:"

while [ -z "$welcomeusermessage" ]
do
  if [ -n "$welcomeUserMessage" ]; then
    echo "Current Welcome User Message:\n$welcomeUserMessage\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter Welcome User Message"
  read welcomeusermessage
  if [ -z "$welcomeusermessage" ]; then
    if [ -n "$welcomeUserMessage" ]; then
      welcomeusermessage=$welcomeUserMessage
    else
      echo "Cannot leave Welcome User Message empty!"
    fi
  fi
done

while [ -z "$welcomeadminmessage" ]
do
  if [ -n "$welcomeAdminMessage" ]; then
    echo "Current Welcome Admin Message:\n$welcomeAdminMessage\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter Welcome Admin Message"
  read welcomeadminmessage
  if [ -z "$welcomeadminmessage" ]; then
    if [ -n "$welcomeAdminMessage" ]; then
      welcomeadminmessage=$welcomeAdminMessage
    else
      echo "Cannot leave Welcome Admin Message empty!"
    fi
  fi
done

while [ -z "$newdevicemessage" ]
do
  if [ -n "$newDeviceMessage" ]; then
    echo "Current New Device Message:\n$newDeviceMessage\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter New Device Message"
  read newdevicemessage
  if [ -z "$newdevicemessage" ]; then
    if [ -n "$newDeviceMessage" ]; then
      newdevicemessage=$newDeviceMessage
    else
      echo "Cannot leave New Device Message empty!"
    fi
  fi
done

while [ -z "$forgotpasswordmessage" ]
do
  if [ -n "$forgotPasswordMessage" ]; then
    echo "Current Forgot Password Message:\n$forgotPasswordMessage\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter Forgot Password Message"
  read forgotpasswordmessage
  if [ -z "$forgotpasswordmessage" ]; then
    if [ -n "$forgotPasswordMessage" ]; then
      forgotpasswordmessage=$forgotPasswordMessage
    else
      echo "Cannot leave Forgot Password Message empty!"
    fi
  fi
done

echo "Configuration of RabbitMQ settings:"

while [ -z "$rabbithost" ]
do
  if [ -n "$host" ]; then
    echo "Current Rabbit Host: $host\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter host"
  read rabbithost
  if [ -z "$rabbithost" ]; then
    if [ -n "$host" ]; then
      rabbithost=$host
    else
      echo "Cannot leave Rabbit Host value empty!"
    fi
  fi
done

while [ -z "$rabbitport" ]
do
  if [ -n "$port" ]; then
    echo "Current Rabbit Port: $port\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter port"
  read rabbitport
  if [ -z "$rabbitport" ]; then
    if [ -n "$port" ]; then
      rabbitport=$port
    else
      echo "Cannot leave Rabbit Port value empty!"
    fi
  fi
done

while [ -z "$rabbituser" ]
do
  if [ -n "$user" ]; then
    echo "Current Rabbit User: $user\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter user"
  read rabbituser
  if [ -z "$rabbituser" ]; then
    if [ -n "$user" ]; then
      rabbituser=$user
    else
      echo "Cannot leave Rabbit User value empty!"
    fi
  fi
done

while [ -z "$rabbitpassword" ]
do
  if [ -n "$password" ]; then
    echo "Current Rabbit Password: $password\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter password"
  read rabbitpassword
  if [ -z "$rabbitpassword" ]; then
    if [ -n "$password" ]; then
      rabbitpassword=$password
    else
      echo "Cannot leave Rabbit Password value empty!"
    fi
  fi
done

while [ -z "$rabbitexchange" ]
do
  if [ -n "$exchangename" ]; then
    echo "Current Rabbit Exchange: $exchangename\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter the Rabbit Exchange"
  read rabbitexchange
  if [ -z "$rabbitexchange" ]; then
    if [ -n "$exchangename" ]; then
      rabbitexchange=$exchangename
    else
      echo "Cannot leave Rabbit Exchange value empty!"
    fi
  fi
done

while [ -z "$rabbitqueue" ]
do
  if [ -n "$queuename" ]; then
    echo "Current Rabbit Queue: $queuename\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter the Rabbit Queue"
  read rabbitqueue
  if [ -z "$rabbitqueue" ]; then
    if [ -n "$queuename" ]; then
      rabbitqueue=$queuename
    else
      echo "Cannot leave Rabbit Queue value empty!"
    fi
  fi
done

while [ -z "$rabbitvirtualhost" ]
do
  if [ -n "$virtualhost" ]; then
    echo "Current Rabbit Virtual Host: $virtualhost\n"
    echo "Hit enter to use existing value"
  fi

  echo "prompt:Please enter the Rabbit Virtual Host"
  read rabbitvirtualhost
  if [ -z "$rabbitvirtualhost" ]; then
    if [ -n "$virtualhost" ]; then
      rabbitvirtualhost=$virtualhost
    else
      echo "Cannot leave Rabbit Virtual Host value empty!"
    fi
  fi
done

#
# Outout to the settings file
#
echo "[basicsettings]" > welcome_config.ini
echo "clientname=\"${CLIENT_NAME}\"" >> welcome_config.ini
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

