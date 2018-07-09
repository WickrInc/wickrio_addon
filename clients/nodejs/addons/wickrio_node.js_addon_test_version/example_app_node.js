var addon = require('bindings')('wickrio_addon'); //Required in every Addon Node.js app
module.exports = addon; //Required in every Addon Node.js app

var client = process.argv[2]; //Required: accepts command line argument for bot client name
console.log(addon.clientInit(client)); //Required: Initializes the client bot

var members = ['wickraaron@wickrautomation.com'];
var message = "Testing time!"
var bor = "60";
var ttl = "100";

var sMessage = addon.cmdSend1to1Message(members, message, ttl , bor); //Calling the Send1To1Message REST API function(bor and ttl are optional)
console.log(sMessage);

//Infinite loop waiting for incoming messgaes into the bot
for(;;){
var rMessage = addon.cmdGetReceivedMessage(); //Calling the GetMessageReceived REST API function
if(rMessage === "{ }" || rMessage === "" || !rMessage){
  continue;
}
else
  console.log(rMessage);
}
