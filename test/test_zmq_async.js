import {Request, Reply} from "zeromq"

/*
const {
    Worker,
    receiveMessageOnPort,
    isMainThread,
    MessageChannel,
} = require("node:worker_threads");
*/
import deasync from 'deasync'

/*const {
  ReceiveMessageCommand,
  DeleteMessageCommand,
  SendMessageCommand,
  SQSClient,
  DeleteMessageBatchCommand,
} = require('@aws-sdk/client-sqs')
*/

// Used to wait for commands to finish
const sleep = (delay) => new Promise((resolve) => setTimeout(resolve, delay))

class ZMQCommands {
  constructor(botname) {
    this.botname = botname
    this.requestQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/0';
    this.asyncQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/2';
    this.heartbeatQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/1';

    this.requestSocket = new Request
    //this.requestSocket.sendTimeout = 5000;
    this.requestSocket.connect(this.requestQName)
    console.log('ZMQCommands: request socket bound to', this.requestQName);

    this.asyncSocket = new Reply
//    this.asyncSocket.receiveTimeout = 0
//    this.asyncSocket.receiveTimeout = 5000;
    this.asyncSocket.receiveTimeout = -1;
//    this.asyncSocket.connect(this.asyncQName)
//    console.log('ZMQCommands: async socket bound to', this.asyncQName);

    this.sendMessageAsync = (message) => 
      new Promise((resolve, reject) => {

        let safeMessage = message.replace(/[\u007f-\uffff]/g,
        function(c) {
          return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
        });

        try {
          this.requestSocket.send(message).then(results => {
            console.log('message sent, now will receive')
            const [msgs] = this.requestSocket.receive().then((result) => {


              if (result === undefined || result.length === 0) {
                resolve( {
                  success : false,
                  return_code : '',
                  result : '',
                });
              }
        
              if (result.length > 1) {
                console.log('sendMessage: response has more than one response!')
              }
        
              if (Array.isArray(result)) {
                console.log('result is an array')
                console.log('result lenght=', result.length)
              }
              const receive_result = result[0];
        
              let response = {};
        
              if (receive_result === undefined) {
                resolve(response);
              }
        
              const receive_msg = receive_result.toString();

              // Parse the error code from teh beginning of the response
              const pos = receive_msg.search(':');
              if (pos === -1) {
                response = {
                  success : true,
                  return_code : '0',
                  result : receive_msg,
                };
                resolve(response);
              }
        
              const retVal = receive_msg.substring(0, pos);
        
              response = {
                success : (retVal === '0'),
                return_code : retVal,
                result : receive_msg.substring(pos+1),
              };
        //    console.log('sendMessage: response=', response)
        
              resolve(response);
            })
            .catch(error => console.err(err))

            return msgs
          }).catch(error => console.err(err))
        } catch (err) {
          console.log(err);
          reject(false);
        }
      })
  }

  async startup() {
    this.asyncSocket.connect(this.asyncQName)
    console.log('ZMQCommands: async socket bound to', this.asyncQName);
  }

  // Wait for a promise without using the await
  wait(promise) {
    var done = 0;
    var result = null;
    promise.then(
      // on value
      function (value) {
          done = 1;
          result = value;
          return (value);
      },
      // on exception
      function (reason) {
          done = 1;
          throw reason;
      }
    );

    while (!done)
        deasync.runLoopOnce();

    return (result);
  }

  async sendMessage(message) {
    /*
    const result = this.sendMessageAsync(message);
    return result
*/

    /*
    var done = true;
    var result = {};

    const sendMessagePromise = this.sendMessageAsync(message).then(
      // on value
      function(value) {
        done = true;
        result = value;
        return value;
      },
      // on exception
      function(reason) {
        done = true;
        throw reason;
      }
    );

    while (!done) {
      deasync.runLoopOnce();
    }
    return result;
    */


    /*
    const send2ZMQ = (message) =>
    new Promise((resolve, reject) => {
      let safeMessage = message.replace(/[\u007f-\uffff]/g,
      function(c) {
        return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
      });

      try {
        this.requestSocket.send(message)
      } catch(err) {
      }
    })


    const recvFromZMQ = () =>
    new Promise((resolve, reject) => {
      const [msgs] = this.requestSocket.receive().then((result) => {
        if (result === undefined || result.length === 0) {
          resolve( {
            success : false,
            return_code : '',
            result : '',
          });
        }
  
        if (result.length > 1) {
          console.log('sendMessage: response has more than one response!')
        }
  
        if (Array.isArray(result)) {
          console.log('result is an array')
          console.log('result lenght=', result.length)
        }
        const receive_result = result[0];
  
        let response = {};
  
        if (receive_result === undefined) {
          resolve(response);
        }
  
        const receive_msg = receive_result.toString();

        // Parse the error code from teh beginning of the response
        const pos = receive_msg.search(':');
        if (pos === -1) {
          response = {
            success : true,
            return_code : '0',
            result : receive_msg,
          };
          resolve(response);
        }
  
        const retVal = receive_msg.substring(0, pos);
  
        response = {
          success : (retVal === '0'),
          return_code : retVal,
          result : receive_msg.substring(pos+1),
        };
  //    console.log('sendMessage: response=', response)
  
        resolve(response);
      })
    })



    var done = true;
    var result = undefined;

    send2ZMQ(message).then(
      // on value
      function () {
          done = true;
          return;
      },
      // on exception
      function (reason) {
          done = true;
          throw reason;
      }
    );

    while (!done)
      deasync.runLoopOnce();

    done = false;
    recvFromZMQ().then(
      // on value
      function (value) {
          done = true;
          result = value;
          return value;
      },
      // on exception
      function (reason) {
          done = true;
          throw reason;
      }
    );

    while (!done)
      deasync.runLoopOnce();
*/


  let safeMessage = message.replace(/[\u007f-\uffff]/g,
  function(c) {
    return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
  });

  try {
    await this.requestSocket.send(message);
  
    console.log('message sent, now will receive')

    const result = await this.requestSocket.receive();

    if (result === undefined || result.length === 0) {
      return( {
        success : false,
        return_code : '',
        result : '',
      });
    }

    if (result.length > 1) {
      console.log('sendMessage: response has more than one response!')
    }

    if (Array.isArray(result)) {
      console.log('result is an array')
      console.log('result lenght=', result.length)
    }
    const receive_result = result[0];

    let response = {};

    if (receive_result === undefined) {
      return( {
        success : false,
        return_code : '500',
        result : 'Response from engine is undefined!',
      });
    }

    const receive_msg = receive_result.toString();

    // Parse the error code from teh beginning of the response
    const pos = receive_msg.search(':');
    if (pos === -1) {
      response = {
        success : true,
        return_code : '0',
        result : receive_msg,
      };
      return(response);
    }

    const retVal = receive_msg.substring(0, pos);

    response = {
      success : (retVal === '0'),
      return_code : retVal,
      result : receive_msg.substring(pos+1),
    };
//    console.log('sendMessage: response=', response)

    return(response);
  } catch(err) {
    console.error(err);

    return( {
      success : false,
      return_code : '500',
      result : err,
    });
  }
  


    /*
    let result = {};
    let workerDone = false;

    const worker = new Worker("./zmq_sendmessage_worker.js", {
      workerData: {
        socket: this.requestSocket,
        message: message,
      },
    });
    worker.on("message", (data) => {
      result = data;
      workerDone = true;
   });
    worker.on("error", (msg) => {
      reject(`An error ocurred: ${msg}`);
      result = {};
      workerDone = true;
    });

    require('deasync').loopWhile(function(){return !workerDone;});

    return result;
    */

    /*
    const sendMessageWorker = (socket, message) =>
      new Promise(function (resolve, reject) {
        const worker = new Worker("./zmq_sendmessage_worker.js", {
          workerData: {
	          socket: socket,
	          message: message,
          },
        });
        worker.on("message", (data) => {
          resolve(data);
       });
        worker.on("error", (msg) => {
          reject(`An error ocurred: ${msg}`);
        });
      });

    const response = await sendMessageWorker(this.requestSocket, message)
    if (response) {
      console.log(response)
      return response
    } else {
      return {}
    }
    */

/*
    this.workerSignal[0] = 0;
    try {
      const subChannel = new MessageChannel();
      this.worker.postMessage({
        signal : this.workerSignal,
        port: subChannel.port1,
        args : { socket: this.requestSocket, message: message },
      }, [
          subChannel.port1
      ]);

      // Sleep until the other thread sets workerSignal[0] to 1
      Atomics.wait(this.workerSignal, 0, 0);

      // Read the message (result) from the worker synchronously
      return receiveMessageOnPort(subChannel.port2).message.result;
    } catch(err) {
      return undefined
    }
    */
  }

  async getMessageOld() {
    try {
      const [msg] = await this.asyncSocket.receive()
      return msg;
    } catch(err) {
      console.error(err);
      return undefined
    }
  }

  async getMessage() {
    try {
      const result = await this.asyncSocket.receive();

      if (result === undefined || result.length === 0) {
        return(undefined);
      }
  
      if (result.length > 1) {
        console.log('getMessage: there are more than one messages!')
      }
  
      // Send response to finish the send/receive sequence
      const message = 'success'
      await this.asyncSocket.send(message);

      if (Array.isArray(result)) {
        console.log('result is an array')
        console.log('result length=', result.length)
      }
      const receive_result = result[0];
  
      if (receive_result === undefined) {
        return undefined
      }
  
      const receive_msg = receive_result.toString();
      return receive_msg
    } catch (err) {
      console.log(err)
      return undefined
    }
  }


  async getMessageWithCallback(callback) {
    try {
      for await (const [msg] of this.asyncSocket) {
        // Send response to finish the send/receive sequence
        const message = 'success'
        await this.asyncSocket.send(message);

        if (Array.isArray(msg)) {
          console.log('result is an array')
          console.log('result length=', msg.length)
          const receive_result = msg[0];
  
          if (receive_result === undefined) {
            return undefined
          }
    
          const receive_msg = receive_result.toString();
  
          callback(receive_msg)
        } else {
          const receive_msg = msg.toString();
          callback(receive_msg)
        }
      }
    } catch (err) {
      console.log(err)
      return undefined
    }
  }

}

//module.exports = ZMQCommands
export default ZMQCommands

