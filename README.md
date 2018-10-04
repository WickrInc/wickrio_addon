# WickrIO API Addon

Wickr's Node.js C++ Addon for a JavaScript API interface

## Usage and example:

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
