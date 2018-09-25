var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "complianceBot";
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
  addon.cmdStartAsyncRecvMessages(listen);

  function listen(message) {
    console.log(message);
    try {
      fs.appendFileSync('receivedMessages.log', message + '\n', 'utf8');
    } catch (err) {
      return console.log(err);
    }
  }
}).catch(error => {
  console.log('Error: ', error);
});
