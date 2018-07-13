var addon = require('bindings')('wickrio_addon');

module.exports = addon;
console.log(addon.clientInit('aaronbot023299@85022943.net'));

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
