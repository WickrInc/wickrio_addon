var addon = require('bindings')('wickrio_addon');

module.exports = addon;
var amqp_user = 'wickr_user';
var amqp_password = 'wickr_user'
var amqp_address = 'localhost';
var amqp_port = '5672';
console.log(addon.clientInit('aaronbot023299@85022943.net', amqp_user, amqp_password, amqp_address, amqp_port));

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
