var addon = require('wickrio_addon');
var prompt = require('prompt');
prompt.start();
module.exports = addon;

return new Promise((resolve, reject) => {
  var schema = {
    properties: {
      client_bot_username: {
        pattern: /^[a-zA-Z0-9@_.]+$/,
        type: 'string',
        description: 'Please enter your client bot\'s username',
        message: 'Client bot username must be entered in order to use Wickr\'s REST API! Username must be only letters, numbers, periods, at(@) signs, or underscores, Please try again.',
        required: true
      }
    }
  };
  if (process.argv[2] === undefined) {
    return new Promise((resolve, reject) => {
      prompt.get(schema, function(err, result) {
        resolve(result.client_bot_username);
      });
    }).then(result => {
      var response = addon.clientInit(result);
      resolve(response);
    }).catch(error => {
      console.log('Error: ', error);
    });
  } else {
    var response = addon.clientInit(process.argv[2]);
    resolve(response);
  }

}).then(result => {
console.log(result);
var vGroupID = "S4b87ee9e90aba8557ace71794438220927fab70b7ef2a242f96688234ac253f";
var members = ['wickraaron@wickrautomation.com'];
var members1;
var moderators = ['wickraaron@wickrautomation.com'];
var bor = "600";
var ttl = "100";
var title = "West World";
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
// console.log(addon.cmdSend1to1Message(members, message, ttl , bor));
console.log(addon.cmdSend1to1Attachment(members, attachment, "", ttl, bor));
console.log(addon.cmdSend1to1Attachment(members, attachmentURL, displayname, ttl, bor));
// console.log(addon.cmdSendRoomMessage(vGroupID, message, ttl, bor));
// console.log(addon.cmdSendRoomAttachment(vGroupID, attachment, "", ttl, bor));
console.log(addon.cmdSendRoomAttachment(vGroupID, attachmentURL, displayname, ttl, bor));
// console.log(addon.closeClient());
}).catch(error => {
      console.log('Error: ', error);
    });
