Need a recent version of node:

1. curl -sL https://deb.nodesource.com/setup_8.x | sudo -E bash -
2. sudo apt-get install -y nodejs
3. node -v


In order to build the WickrIO Node.js addon you will need to have the following installed:

install cmake:
- sudo apt-get install cmake

install g++:
- sudo apt-get install g++



To build the WickrIO Node.js addon do the following:
1. cd to the addon directory
2. run "npm install"

Running a sample:
1. cd to the specific example
2. run "./install.sh"
3. run "./start.sh"
  a. Optional: can run sample app with client bot's name like this: "node <sample.js file> <client_bot_username>"



Links:

https://github.com/creationix/nvm/blob/master/README.md#usage
