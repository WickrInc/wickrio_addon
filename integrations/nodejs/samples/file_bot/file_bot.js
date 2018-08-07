var addon = require('wickrio_addon');
var prompt = require('prompt');
var fs = require('fs');
var fileExists = require('file-exists');
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
        if (rMessage.message) {
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
            var attachment = rMessage.message.substr(rMessage.message.indexOf(' ') + 1);
            try {
              var as = await fs.accessSync('files/' + attachment, fs.constants.R_OK | fs.constants.W_OK);
              console.log('can access', attachment);
            } catch (err) {
              var msg = attachment + ' does not exist!';
              console.error(msg);
              var sMessage = await addon.cmdSend1to1Message(userArr, msg, ttl, bor);
              console.log(sMessage);
              continue;
            }
            console.log(addon.cmdSend1to1Attachment(userArr, __dirname + '/files/' + attachment, attachment, ttl, bor));
          } else if (request[0] === '/delete') {
            var attachment = rMessage.message.substr(rMessage.message.indexOf(' ') + 1);
            if (attachment === '*') {
              var msg = "Sorry, I'm not allowed to delete all the files in the directory.";
              var sMessage = await addon.cmdSend1to1Message(userArr, msg, ttl, bor);
              console.log(sMessage);
              continue;
            }
            try {
              var os = await fs.statSync('files/' + attachment);
            } catch (err) {
              var msg = attachment + ' does not exist!';
              console.error(msg);
              var sMessage = await addon.cmdSend1to1Message(userArr, msg, ttl, bor);
              console.log(sMessage);
              continue;
            }
            try {
              var rm = await fs.unlinkSync('files/' + attachment);
            } catch (err) {
              if (err) {
                throw err;
                var sMessage = await addon.cmdSend1to1Message(userArr, err, ttl, bor);
                console.log(sMessage);
                continue;
              }
            }
            var msg = "File named: '" + attachment + "' was deleted successfully!";
            var sMessage = await addon.cmdSend1to1Message(userArr, msg, ttl, bor);
            console.log(sMessage);
          } else if (request[0] === '/help') {
            var help = "/help - List all available commands\n" +
              "/list - Lists all available files\n" +
              "/get FILE_NAME - Retrieve the specified file\n" +
              "/delete FILE_NAME - Deletes the specified file\n";
            var sMessage = addon.cmdSend1to1Message(userArr, help, ttl, bor);
            console.log(sMessage);
          }
        } else if (rMessage.file && JSON.stringify(rMessage) !== JSON.stringify(prevMessage)) {
          for (;;) {
            try {
              var os = await fileExists.sync(rMessage.file.localfilename);
              if(exists)
                break;
              else
                continue;
            } catch (err) {
              console.error(err);
            }
          }
          var cp = await fs.copyFileSync(rMessage.file.localfilename, 'files/' + rMessage.file.filename);
          var msg = "File named: '" + rMessage.file.filename + "' successfully saved to directory!";
          var sMessage = await addon.cmdSend1to1Message(userArr, msg, ttl, bor);
          var prevMessage = rMessage;
          console.log(sMessage);
        } else{
          console.log(rMessage);
          continue;
        }
      }
  }
}).catch(error => {
  console.log('Error: ', error);
});
