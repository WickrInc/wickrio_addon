var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "complianceBot";
module.exports = addon;
process.stdin.resume(); //so the program will not close instantly

function exitHandler(options, err) {
  if (err) {
    console.log(err.stack);
    console.log(addon.closeClient());
    process.exit();
  }
  if (options.exit) {
    console.log(addon.closeClient());
    process.exit();
  } else if (options.pid) {
    console.log(addon.closeClient());
    process.kill(process.pid);
  }
}

//catches ctrl+c and stop.sh events
process.on('SIGINT', exitHandler.bind(null, {
  exit: true
}));

// catches "kill pid" (for example: nodemon restart)
process.on('SIGUSR1', exitHandler.bind(null, {
  pid: true
}));
process.on('SIGUSR2', exitHandler.bind(null, {
  pid: true
}));

//catches uncaught exceptions
process.on('uncaughtException', exitHandler.bind(null, {
  exit: true
}));

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
