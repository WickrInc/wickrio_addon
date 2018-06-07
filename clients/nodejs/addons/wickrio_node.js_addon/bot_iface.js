var addon = require('bindings')('wickrio_addon');

module.exports = addon;
addon.clientInit('aaronbot019512@62114373.net');
var members = ['wickraaron@wickrautomation.com', 'wickrpak@wickrautomation.com'];
var moderators = ['wickraaron@wickrautomation.com', 'wickrpak@wickrautomation.com'];
var bor = 6;
var ttl = 0;
var title = "North room";
var description = "This is a room";
// addon.cmdAddGroupConvo(members, bor, ttl);
console.log(addon.cmdAddRoom(members, moderators, title, description, ttl, bor));
console.log(addon.cmdGetStatistics());
console.log(addon.cmdGetRooms());
console.log(addon.cmdGetGroupConvos());
console.log(addon.cmdGetReceivedMessage());
