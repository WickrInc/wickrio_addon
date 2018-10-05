# WickrIO API Addon

Wickr's Node.js C++ Addon for a JavaScript API interface

## Description:

This is the Node.js C++ Addon that provides access to Wickr's bot client.  This WickrIO addon interface supports a set of Wickr client functionality that you can access via your Javascript code. You will be able to do the following Wickr operations:

* Send and receive messages and files
* Create, modify, leave, delete and configure secure rooms
* Create and leave group conversations

Before you can use the WickrIO addon, you will need to have Wickr's bot client. The WickrIO bot packages contain several sample bots that use this addon.

NOTE: The WickrIO bot package is not currently available to the general public. 

## Usage and example:

Interaction with the WickrIO Node.js addon has the following sequence of operations:

1. You will need a "require('wickrio_addon') statement at the beginning of your javascript.
2. You will need to initialize the WickrIO Node.js addon interface, but calling the API's clientInit() function. This API requires the name of the WickrIO client as an argument. This identifies which client the addon will interact with.
3. Interact with the WickrIO client using the WickrIO Node.js addon APIs.

The following is an example of how to interact with the WickrIO bot client using the WickrIO Node.hs addon:

```javascript
var addon = require('wickrio_addon');
module.exports = addon;

var client = "bot_username";
var response = addon.clientInit(client);

var vGroupID = "S9d0b36ac51c5a71f8d8dd28b8614c273084fd9785c22788df672fb6c8e0ae88";
var members = ['wickr_username'];
var moderators = ['wickr_username'];
var bor = "600";  //OPTIONAL
var ttl = "1000"; //OPTIONAL
var title = "Example Room";
var description = "The Good Room";
var message = "Testing time!"
var attachmentURL = "<https://www.alsop-louie.com/wp-content/uploads/2017/03/wickr-logo-2-crop.png>"
var displayname = "Logo.png";

var cmd1 = addon.cmdSend1to1Message(members, message, ttl, bor);  
console.log(cmd1); //if successful should print "Sending message"

var cmd2 = addon.cmdAddRoom(members, moderators, title, description, ttl, bor);
console.log(cmd2); //if successful should print a json with vgroupid of the newly created room

var cmd3 = addon.cmdSendRoomAttachment(vGroupID, attachmentURL, displayname); //Notice: ttl and bor arguments are omitted and command will still work
console.log(cmd3); //if successful should print "Sending message"

console.log(addon.closeClient());
```
