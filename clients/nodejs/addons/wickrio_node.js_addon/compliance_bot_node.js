var addon = require('bindings')('wickrio_addon');
var fs = require('fs');
var prompt = require('prompt');
prompt.start();
module.exports = addon;

return new Promise((resolve, reject) => {
prompt.get(['client_bot_username'], function (err, result) {
   var response = addon.clientInit(result.client_bot_username);
   resolve(response);
 });
}).then(result => {
console.log(result);

for (;;) {
  var message = addon.cmdGetReceivedMessage();
  if(message === "{ }" || message === "" || !message){
    continue;
  }
  else{
    console.log(message);
    fs.writeFile("receivedMessages.log", message, function(err){
      if(err)
        return console.log(err);
    });
  }
}
}).catch(error => {
      console.log('Error: ', error);
    });
