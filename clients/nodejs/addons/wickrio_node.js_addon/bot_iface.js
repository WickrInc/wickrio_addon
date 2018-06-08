var addon = require('bindings')('wickrio_addon');

module.exports = addon;
console.log(addon.clientInit('aaronbot019512@62114373.net'));
var vGroupID = "G305c1a889421a809291dfee69ad41449756df5333bb0d1eba9e774d04a32922";
var members = ['wickraaron@wickrautomation.com', 'wickrpak@wickrautomation.com'];
var moderators = ['wickraaron@wickrautomation.com', 'aaronbot019512@62114373.net'];
var bor = "6";
var ttl = "0";
var title = "New Name";
var description = "room";
//console.log(addon.cmdAddRoom(members, moderators, title, description, ttl, bor));
//console.log(addon.cmdGetStatistics());
//console.log(addon.cmdGetRoom(vGroupID));
//console.log(addon.cmdAddGroupConvo(members, ttl, bor));
//console.log(addon.cmdModifyRoom(vGroupID, members, moderators, title, description, ttl, bor));
console.log(addon.cmdGetGroupConvo(vGroupID));
console.log(addon.cmdDeleteGroupConvo(vGroupID));
//console.log(addon.cmdGetReceivedMessage());
