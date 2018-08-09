var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "exampleApp";
module.exports = addon;

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
