var addon = require('wickrio_addon');
var prompt = require('prompt');
prompt.start();
process.title = "exampleApp";
module.exports = addon;

return new Promise((resolve, reject) => {
  var schema = {
    properties: {
      client_bot_username: {
        pattern: /^[a-zA-Z0-9@_.]+$/,
        type: 'string',
        description: 'Please enter your client bot\'s username',
        message: 'Client bot username must be entered in order to use Wickr\'s REST API! Username must be only letters, numbers, periods, at(@) signs, or underscores, Please try again.',
        required: true
      }
    }
  };
  if (process.argv[2] === undefined) {
    return new Promise((resolve, reject) => {
      prompt.get(schema, function(err, result) {
        resolve(result.client_bot_username);
      });
    }).then(result => {
      var response = addon.clientInit(result);
      resolve(response);
    }).catch(error => {
      console.log('Error: ', error);
    });
  } else {
    var response = addon.clientInit(process.argv[2]);
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
