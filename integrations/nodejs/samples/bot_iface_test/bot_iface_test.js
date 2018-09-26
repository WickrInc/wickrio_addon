var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "ifaceTest";
module.exports = addon;
process.stdin.resume(); //so the program will not close instantly

function exitHandler(options, err) {
  if (err) {
    console.log(err.stack);
    console.log(addon.closeClient());
    return process.exit();
  }
  if (options.exit) {
    console.log(addon.closeClient());
    return process.exit();
  } else if (options.pid) {
    console.log(addon.closeClient());
    return process.kill(process.pid);
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

return new Promise(async (resolve, reject) => {
  if (process.argv[2] === undefined) {
    var client = await fs.readFileSync('client_bot_username.txt', 'utf-8');
    client = client.trim();
    var response = await addon.clientInit(client);
    resolve(response);


  } else {
    var response = await addon.clientInit(process.argv[2]);
    resolve(response);
  }

}).then(result => {
console.log(result);
var vGroupID = "S90ee5a3ea4c27d619636e6e83f074dfa9a13090f23ae5d31fe9604830e13034";
var members = ['wickraaron@wickrautomation.com'];
var members1;
var moderators = ['wickraaron@wickrautomation.com'];
var bor = "600";
var ttl = "100";
var title = "East Coast";
var description = "The Good Room";
var message = "Testing time!"
var attachment = "/opt/WickrIODebug/package.json";
var attachmentURL = "https://www.alsop-louie.com/wp-content/uploads/2017/03/wickr-logo-2-crop.png"
var displayname = "Logo.png";

// console.log(addon.cmdAddRoom(members, moderators, title, description, ttl, bor));
// console.log(addon.cmdDeleteRoom(vGroupID));
// console.log(addon.cmdGetStatistics());
// console.log(addon.cmdClearStatistics());
// console.log(addon.cmdGetRoom(vGroupID));
// console.log(addon.cmdGetRooms());
// console.log(addon.cmdLeaveRoom(vGroupID));
// console.log(addon.cmdAddGroupConvo(members, ttl, bor));
// console.log(addon.cmdModifyRoom(vGroupID, members, moderators, title, description, ttl, bor));
// console.log(addon.cmdGetGroupConvo(vGroupID));
// console.log(addon.cmdGetGroupConvos());
// console.log(addon.cmdDeleteGroupConvo(vGroupID));
// console.log(addon.cmdGetReceivedMessage());
console.log(addon.cmdSend1to1Message(members, message, ttl , bor));
// console.log(addon.cmdSend1to1Attachment(members, attachment, "", ttl, bor));
// console.log(addon.cmdSend1to1Attachment(members, attachmentURL, displayname, ttl, bor));
// console.log(addon.cmdSendRoomMessage(vGroupID, message, ttl, bor));
// console.log(addon.cmdSendRoomAttachment(vGroupID, attachment, "", ttl, bor));
// console.log(addon.cmdSendRoomAttachment(vGroupID, attachmentURL, displayname, ttl, bor));
console.log(addon.closeClient());
}).catch(error => {
      console.log('Error: ', error);
    });
