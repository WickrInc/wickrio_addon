var addon = require('bindings')('wickrio_addon');
var fs = require('fs');

module.exports = addon;
var amqp_user = 'wickr_user';
var amqp_password = 'wickr_user'
var amqp_address = 'localhost';
var amqp_port = '5672';
console.log(addon.clientInit('aaronbot023299@85022943.net', amqp_user, amqp_password, amqp_address, amqp_port));

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
