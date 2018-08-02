var addon = require('wickrio_addon');
var prompt = require('prompt');
var fs = require('fs');
prompt.start();
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

}).then(async result => {
  console.log(result);
  // var sMessage = addon.cmdSend1to1Message(members, message, ttl , bor);
  // console.log(sMessage);

  //Infinite loop waiting for incoming messgaes into the bot
  for (;;) {
    try {
      var rMessage = await addon.cmdGetReceivedMessage();
      if (rMessage === "{ }" || rMessage === "" || !rMessage) {
        continue;
      } else {
        rMessage = JSON.parse(rMessage);
        var request = rMessage.message;
        var sender = rMessage.sender;
        var userArr = [];
        userArr.push(sender);

      }
    } catch (err) {
      console.log(err);
    }
    // var received = await JSON.parse(rMessage);
    if (rMessage === "{ }" || rMessage === "" || !rMessage) {
      continue;
    } else if (request === '/list') {
      var fileArr = [];
      var bor = "9000";
      var ttl = "9000";
      fileArr.push('List of files in the given directory:');
      var answer;
      fs.readdirSync('.').forEach(file => {
        console.log(file);
        fileArr.push(file.toString());
      });
      fileArr = fileArr.join('\n');
      var sMessage = addon.cmdSend1to1Message(userArr, fileArr, ttl, bor);
      console.log(sMessage);
    } else if (request === '/get') {
      var attachment = request.split(" ");
      console.log(addon.cmdSend1to1Attachment(userArr, attachment[1], "", ttl, bor));
    } else
      console.log(rMessage);
  }
}).catch(error => {
  console.log('Error: ', error);
});
