# WickrIO API Addon

This is the Node.js low level APIs to the WickrIO bot engine

## Description:

This is the Node.js low level APIs that provide access to the WickrIO's bot client.  You will be able to do at least the following operations:

* Send and receive messages and files
* Create, modify, leave, delete and configure secure rooms
* Create and leave group conversations

For full documentation please visit: [https://wickrinc.github.io/wickrio-docs/#developing-integrations-node-js-addon-api](https://wickrinc.github.io/wickrio-docs/#developing-integrations-node-js-addon-api)

Before you can use the WickrIO addon, you will need to have Wickr's bot client. The WickrIO bot packages contain several sample bots that use this addon.

NOTE: Versions 7.1.1 and above do not rely on a specific version of node and the Wickrio Addon uses ZeroMq making the WickrIO Bot APIs asynchronous. Please use this [example](https://wickrinc.github.io/wickrio-docs/#installation-version-6-48-announcement) code to update your custom integration. 

## Usage and example:

Interaction with the WickrIO Node.js addon has the following sequence of operations:

1. You will need a "require('wickrio_addon') statement at the beginning of your javascript.
2. You will need to initialize the WickrIO Node.js addon interface, but calling the API's clientInit() function. This API requires the name of the WickrIO client as an argument. This identifies which client the addon will interact with.
3. Interact with the WickrIO client using the WickrIO Node.js addon APIs.

The following is an example of how to interact with the WickrIO bot client using the WickrIO Node.js addon and Bot API toolkit:

```javascript
const WickrIOAPI = require('wickrio_addon');
const WickrIOBotAPI = require('wickrio-bot-api');
const WickrUser = WickrIOBotAPI.WickrUser;

process.stdin.resume(); //so the program will not close instantly

var bot, tokens, bot_username, bot_client_port, bot_client_server;
var tokens = JSON.parse(process.env.tokens);

async function exitHandler(options, err) {
  var closed = await bot.close();
  console.log(closed);
  if (err) {
    console.log("Exit Error:", err);
    process.exit();
  }
  if (options.exit) {
    process.exit();
  } else if (options.pid) {
    process.kill(process.pid);
  }
}

//catches ctrl+c and stop.sh events
process.on('SIGINT', exitHandler.bind(null, {
  exit: true
}));

// catches "kill pid" (for example: nodemon restart)
process.on('SIGUSR1', exitHandler.bind(null, {
  pid: true
}));
process.on('SIGUSR2', exitHandler.bind(null, {
  pid: true
}));

//catches uncaught exceptions
process.on('uncaughtException', exitHandler.bind(null, {
  exit: true
}));

async function main() {
  try {
    var status;
    if (process.argv[2] === undefined) {
      bot_username = tokens.BOT_USERNAME.value;
      bot = new WickrIOBotAPI.WickrIOBot();
      status = await bot.start(bot_username)
    } else {
      bot = new WickrIOBotAPI.WickrIOBot();
      status = await bot.start(process.argv[2])
    }
    console.log(status)
    if (!status)
      exitHandler(null, {
        exit: true,
        reason: 'Client not able to start'
      });
    ///////////////////////
    //Start coding below
    ///////////////////////
    await bot.startListening(listen); //Passes a callback function that will receive incoming messages into the bot client
  } catch (err) {
    console.log(err);
  }
}


async function listen(message) {
  try {
    var parsedMessage = bot.parseMessage(message); //Parses an incoming message and returns and object with command, argument, vGroupID and Sender fields
    if (!parsedMessage) {
      return;
    }
    console.log('parsedMessage:', parsedMessage);
    var wickrUser;
    var command = parsedMessage.command;
    var message = parsedMessage.message;
    var argument = parsedMessage.argument;
    var userEmail = parsedMessage.userEmail;
    var vGroupID = parsedMessage.vgroupid;
    var convoType = parsedMessage.convoType;
    var personal_vGroupID = "";
    if (convoType === 'personal')
      personal_vGroupID = vGroupID;
    var found = bot.getUser(userEmail); //Check if a user exists in the database and get his position in the database
    if (!found) {
      wickrUser = new WickrUser(userEmail, {
        index: 0,
        personal_vGroupID: personal_vGroupID,
        command: "",
        argument: ""
      });
      var added = bot.addUser(wickrUser);
      console.log(added);
    }
    var user = bot.getUser(userEmail);
    user.token = "example_token_A1234";

    //how to determine the command a user sent and handling it
    if (command === '/help') {
      var reply = "What can I help you with?";
      var sMessage = WickrIOAPI.cmdSendRoomMessage(vGroupID, reply); //Respond back to the user or room with a message(using vGroupID)
      var users = [userEmail];
      var sMessage = WickrIOAPI.cmdSend1to1Message(users, reply); //Respond back to the user(using user wickrEmail)
      console.log(sMessage);
    }
  } catch (err) {
    console.log(err);
  }
}

main();
```

# License

This software is distributed under the [Apache License, version 2.0](https://www.apache.org/licenses/LICENSE-2.0.html)

```
   Copyright 2024 Wickr, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
```
