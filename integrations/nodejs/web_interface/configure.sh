#!/bin/sh

 echo "prompt: Please enter your client bot's username:"
 while [ -z "$input" ]
  do
   read  input
   if [ ! -z "$input" ]
    then
     echo ${input} >client_bot_username.txt
   else
     echo "Cannot leave client bot's username empty! Please enter a value:"
   fi
 done
