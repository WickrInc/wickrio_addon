#!/bin/sh

#
# If the input argument exists then check if it is a file
# if so it should contain a list of key=value entries.
#
if [ -n "$1" ]; then
  if [ -f "$1" ]; then
    echo going to use $1
    . "$1"
  fi
fi

if [ -z "$CLIENT_NAME" ]; then
  echo "prompt: Please enter your client bot's username:"
  while [ -z "$input" ]
  do
    read  input
    if [ ! -z "$input" ]
    then
       echo ${input} >client_bot_info.txt
    else
      echo "Cannot leave client bot's username empty! Please enter a value:"
    fi
  done
else
   echo $CLIENT_NAME >client_bot_info.txt
fi

if [ -z "$LISTEN_PORT" ]; then
  echo "prompt: Please enter your client bot's port:"
  while [ -z "$input2" ]
  do
    read  input2
    if [ ! -z "$input2" ]
    then
      echo 'BOT_PORT='${input2} >>client_bot_info.txt
    else
      echo "Cannot leave client bot's port empty! Please enter a value:"
    fi
  done
else
  echo 'BOT_PORT='${LISTEN_PORT} >>client_bot_info.txt
fi

if [ -z "$API_KEY" ]; then
  echo "prompt: Please enter your client bot's API-Key:"
  while [ -z "$input3" ]
  do
    read  input3
    if [ ! -z "$input3" ]
    then
      echo 'BOT_API_KEY='${input3} >>client_bot_info.txt
    else
      echo "Cannot leave client bot's API-Key empty! Please enter a value:"
    fi
  done
else
  echo 'BOT_API_KEY='${API_KEY} >>client_bot_info.txt
fi

if [ -z "$API_AUTH_TOKEN" ]; then
 echo "prompt: Please create an Web API Basic Authorization Token(we recommend an alphanumeric string with at least 24 characters):"
  while [ -z "$input4" ]
   do
    read  input4
    if [ ! -z "$input4" ]
     then
      echo 'BOT_API_AUTH_TOKEN='${input4} >>client_bot_info.txt
     else
       echo "Cannot leave Basic Authorization Token empty! Please enter a value:"
    fi
  done
else
  echo 'BOT_API_AUTH_TOKEN='${API_AUTH_TOKEN} >>client_bot_info.txt
fi
