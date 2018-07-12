var addon = require('bindings')('wickrio_addon');
var fs = require('fs');
var prompt = require('prompt');
prompt.start();
module.exports = addon;

return new Promise((resolve, reject) => {
  var schema = {
    properties: {
      client_bot_username: {
        pattern: /^[a-zA-Z0-9@_]+$/,
        type: 'string',
        description: 'Please enter your client bot\'s username',
        message: 'Client bot username must be entered in order to use Wickr\'s REST API! Username must be only letters, numbers, at(@) signs, or underscore, Please try again.',
        required: true
      }
    }
  };
prompt.get(schema, function (err, result) {
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
