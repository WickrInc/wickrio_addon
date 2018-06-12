var addon = require('bindings')('wickrio_addon');
var fs = require('fs');

module.exports = addon;
console.log(addon.clientInit('aaronbot019512@62114373.net'));
console.log(addon.cmdGetGroupConvos());

console.log(addon.cmdGetReceivedMessage());
for (;;) {
  var message = addon.cmdGetReceivedMessage();
  if(message === "{ }"){
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
