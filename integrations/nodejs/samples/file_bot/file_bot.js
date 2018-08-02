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
  //Infinite loop waiting for incoming messgaes into the bot
  for (;;) {
    try {
      var rMessage = addon.cmdGetReceivedMessage();
      if (rMessage === "{ }" || rMessage === "" || !rMessage) {
        continue;
      } else {
        var bor = "9000";
        var ttl = "9000";
        rMessage = JSON.parse(rMessage);
        var sender = rMessage.sender;
        var userArr = [];
        userArr.push(sender);
      }
    } catch (err) {
      console.log(err);
    }
    if (rMessage === "{ }" || rMessage === "" || !rMessage) {
      continue;
    } else if (rMessage.message) {
      var request = rMessage.message.split(" ");
      if (request[0] === '/list') {
        var fileArr = [];
        fileArr.push('List of files in the given directory:');
        var answer;
        fs.readdirSync('files/').forEach(file => {
          fileArr.push(file.toString());
        });
        fileArr = fileArr.join('\n');
        var sMessage = addon.cmdSend1to1Message(userArr, fileArr, ttl, bor);
        console.log(sMessage);
      } else if (request[0] === '/get') {
        var attachment = request[1].toString().trim();
        console.log('attachment:', attachment);
        console.log(addon.cmdSend1to1Attachment(userArr, __dirname + '/files/' + attachment, "", ttl, bor));
      } else if (request[0] === '/help') {
        var help = "/help - List all available commands\n" +
         "/list - List all available files\n" +
         "/get FILE_NAME - Retrieve the specified file\n";
        var sMessage = addon.cmdSend1to1Message(userArr, help, ttl, bor);
      }
    } else if (rMessage.file) {
      // console.log(rMessage.file.localfilename);
      for (;;) {
        try {
          var os = await fs.statSync(rMessage.file.localfilename);
          break;
        } catch (err) {
          continue;
        }
      }
      var cp = await fs.copyFileSync(rMessage.file.localfilename, 'files/' + rMessage.file.filename);
      var msg = "File named: '"+ rMessage.file.filename + "' successfully saved to directory!";
      var sMessage = addon.cmdSend1to1Message(userArr, msg, ttl, bor);
      console.log(sMessage);
    } else
      console.log(rMessage);
  }
}).catch(error => {
  console.log('Error: ', error);
});
