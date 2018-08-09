var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "complianceBot";
module.exports = addon;

return new Promise((resolve, reject) => {
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
