WickrIO Node.JS Native Module Addon Instructions:


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
