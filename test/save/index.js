const ZMQCommands = require('./test_zmq_async')

let zmqCommands = new ZMQCommands('paulcus-bcast-20k-bot')

//so the program will not close instantly
process.stdin.resume();

// Used to wait for commands to finish
const sleep = (delay) => new Promise((resolve) => setTimeout(resolve, delay))

async function exitHandler(options, err) {
  try {
//    var closed = await bot.close();


    const result = await zmqCommands.sendMessage('{ "action" : "stop_async_messages" }');

    if (err || options.exit) {
      console.log("Exit reason:", err);
      process.exit();
    } else if (options.pid) {
      process.kill(process.pid);
    }
  } catch (err) {
    console.log(err);
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

const notDone = true;

function callback(message) {
  console.log('message=', message)
}

async function main() {
  const result = await zmqCommands.sendMessage('{ "action" : "start_async_messages" }');

  while (notDone) {
    try {
//      const message = await zmqCommands.getMessage();
      await zmqCommands.getMessageWithCallback(callback)
    } catch (err) {
      console.log(err);
    }
  }
}

main();

/*
  async messages_callback() {
    return new Promise(async function(resolve, reject) {
        message = await zmqCommands.getMessage();

        if (message != undefined) {
          if (asyncInfo.messageCallback != undefined)
            asyncInfo.messageCallback(message);
        } else {
          if (this.testcount > 0) {
	    this.testcount--;

            if (asyncInfo.messageCallback != undefined) {
              asyncInfo.messageCallback('this is a test message!');
              throw 'messages_callback: sent test message!';
	          }
	        }
	      }
      }
      resolve();
    });
  }
  */
