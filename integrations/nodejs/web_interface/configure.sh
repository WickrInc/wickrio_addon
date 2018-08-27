#!/bin/sh

 echo "prompt: Please enter your client bot's username:"
 while [ -z "$input" ]
  do
   read  input
   if [ ! -z "$input" ]
    then
     echo 'BOT_USERNAME='${input} >client_bot_info.txt
   else
     echo "Cannot leave client bot's username empty! Please enter a value:"
   fi
 done
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
