WickrIO Node.JS Native Module Addon Instructions:



Installation:
1. Run 'npm install' in current directory


At the top of every Node.js file/app you need to include the following lines in order to talk to the WickrIO REST API:

var addon = require('bindings')('wickrio_addon');
module.exports = addon;

var client = process.argv[2]; //Required: accepts command line argument for bot client name
console.log(addon.clientInit(client)); //Required: Initializes the client bot

Then, in order to call any WickrIO REST API function you need to use 'addon' variable, for example:

console.log(addon.cmdAddGroupConvo(members, ttl, bor));
console.log(addon.cmdModifyRoom(vGroupID, members, moderators, title, description, ttl, bor));
console.log(addon.cmdSendRoomMessage(vGroupID, message, ttl, bor));
console.log(addon.cmdGetRooms());



To Run the apps, inside the directory containing the file run:
'node app_name.js'




WickrIO REST API Functions:

addon.cmdAddRoom(members, moderators, title, description, ttl, bor)
addon.cmdDeleteRoom(vGroupID)
addon.cmdGetStatistics()
addon.cmdClearStatistics()
addon.cmdGetRoom(vGroupID)
addon.cmdGetRooms()
addon.cmdLeaveRoom(vGroupID)
addon.cmdAddGroupConvo(members, ttl, bor)
addon.cmdModifyRoom(vGroupID, members, moderators, title, description, ttl, bor)
addon.cmdGetGroupConvo(vGroupID)
addon.cmdGetGroupConvos()
addon.cmdDeleteGroupConvo(vGroupID)
addon.cmdGetReceivedMessage()
addon.cmdSend1to1Message(members, message, ttl , bor)
addon.cmdSend1to1Attachment(members, attachment, ttl , bor)
addon.cmdSendRoomMessage(vGroupID, message, ttl, bor)
addon.cmdSendRoomAttachment(vGroupID, attachment, ttl, bor)
addon.closeClient()
