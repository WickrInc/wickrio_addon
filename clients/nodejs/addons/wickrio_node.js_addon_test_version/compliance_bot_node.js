var addon = require('bindings')('wickrio_addon');
var fs = require('fs');

module.exports = addon;

var client = process.argv[2];
console.log(addon.clientInit(client));

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
