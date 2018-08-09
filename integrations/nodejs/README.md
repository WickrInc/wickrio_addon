Need a recent version of node:

1. sudo apt-get install -y gnupg curl
2. curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -
3. sudo apt-get install -y nodejs
4. Verify NodeJS is installed(make sure it's version 10+): node -v
5. Verify npm is installed: npm -v
  a. if it isn't, run 'curl https://www.npmjs.com/install.sh | sh'

In order to build the WickrIO Node.js addon you will need to have the following installed:

install cmake and g++:
- sudo apt-get install cmake g++ -y


To build the WickrIO Node.js addon do the following:
1. cd to the wickrio_addon directory
2. run "npm install"

Running a sample:
1. cd to the desired sample
2. run "./install.sh"
3. run "./configure.sh"
3. run "./start.sh"
  a. Optional: can run sample app with client bot's name like this: "node <sample.js file> <client_bot_username>"



Links:

https://github.com/creationix/nvm/blob/master/README.md#usage
