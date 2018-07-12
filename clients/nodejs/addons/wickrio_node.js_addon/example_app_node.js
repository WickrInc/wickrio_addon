var addon = require('bindings')('wickrio_addon');
var prompt = require('prompt');
prompt.start();
module.exports = addon;

return new Promise((resolve, reject) => {
prompt.get(['client_bot_username'], function (err, result) {
   console.log('Command-line input received:');
   console.log('username: ' + result.client_bot_username);
   var response = addon.clientInit(result.client_bot_username);
   resolve(response);
 });
}).then(result => {
console.log(result);
var members = ['wickraaron@wickrautomation.com'];
var message = "Testing time!"
var bor = "60"; //optional
var ttl = "100"; //optional

var sMessage = addon.cmdSend1to1Message(members, message, ttl , bor);
console.log(sMessage);

//Infinite loop waiting for incoming messgaes into the bot
for(;;){
var rMessage = addon.cmdGetReceivedMessage();
if(rMessage === "{ }" || rMessage === "" || !rMessage){
  continue;
}
else
  console.log(rMessage);
}
}).catch(error => {
      console.log('Error: ', error);
    });
