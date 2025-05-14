const zmq = require("zeromq")
const { Mutex } = require('async-mutex')

const mutex = new Mutex();

/*
const {
    Worker,
    receiveMessageOnPort,
    isMainThread,
    MessageChannel,
} = require("node:worker_threads");
*/
const deasync = require('deasync')

/*const {
  ReceiveMessageCommand,
  DeleteMessageCommand,
  SendMessageCommand,
  SQSClient,
  DeleteMessageBatchCommand,
} = require('@aws-sdk/client-sqs')
*/

class ZMQCommands {
  constructor(botname, debugOn) {
    this.botname = botname
    this.requestQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/0';
    this.asyncQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/2';
    this.heartbeatQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/1';

    this.sendInProgress = false
    this.lastSentMessage = ''
    this.debug = debugOn

    this.requestSocket = new zmq.Request
    this.requestSocket.sendTimeout = -1;
    this.requestSocket.connect(this.requestQName)
    console.log('ZMQCommands: request socket bound to', this.requestQName);

    this.asyncSocket = new zmq.Reply
//    this.asyncSocket.receiveTimeout = 0
//    this.asyncSocket.receiveTimeout = 5000;
    this.asyncSocket.receiveTimeout = -1;
//    this.asyncSocket.connect(this.asyncQName)
//    console.log('ZMQCommands: async socket bound to', this.asyncQName);
    this.asyncSocket.connect(this.asyncQName)
    console.log('ZMQCommands: async socket bound to', this.asyncQName);

    this.sendMessageAsync = (message) => {
      new Promise((resolve, reject) => {

        let safeMessage = message.replace(/[\u007f-\uffff]/g,
        function(c) {
          return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
        });

        try {
          this.requestSocket.send(message).then(results => {
            if (this.debug) console.log('message sent, now will receive')
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
                if (this.debug) console.log('result is an array')
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
        //    if (this.debug) console.log('sendMessage: response=', response)
        
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

    const release = await mutex.acquire();
    let response = {};
    let result = {}

    let safeMessage = message.replace(/[\u007f-\uffff]/g,
      function (c) {
        return '\\u' + ('0000' + c.charCodeAt(0).toString(16)).slice(-4);
      });

    try {
      
      console.log('Sending message:, ', JSON.parse(message).action);

      await this.requestSocket.send(message);

      console.log('message sent, now will receive')
      result = await this.requestSocket.receive();

      // Save the current message just in case
      this.lastSentMessage = message


    } catch (err) {
      console.error(err);
      console.error('last message:', this.lastSentMessage)

      return ({
        success: false,
        return_code: '500',
        result: err,
      });
    }
    finally {
      release()
    }

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
        if (this.debug) console.log('result is an array')
        }

    const receive_result = result[0]
      if (receive_result === undefined) {
        return ({
          success: false,
          return_code: '500',
          result: 'Response from engine is undefined!',
        });
      }

      const receive_msg = receive_result.toString();

      // Parse the error code from teh beginning of the response
      const pos = receive_msg.search(':');
      if (pos === -1) {
        response = {
          success: true,
          return_code: '0',
          result: receive_msg,
        };
        return (response);
      }

      const retVal = receive_msg.substring(0, pos);

      response = {
        success: (retVal === '0'),
        return_code: retVal,
        result: receive_msg.substring(pos + 1),
      };
      return (response);
  }

  sendMessageAsyncGood(message) {
    return new Promise((resolve, reject) => {

      let safeMessage = message.replace(/[\u007f-\uffff]/g,
      function(c) {
        return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
      });

      try {
        this.requestSocket.send(message).then(results => {
          if (this.debug) console.log('message sent, now will receive')
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
              if (this.debug) console.log('result is an array')
              if (this.debug) console.log('result length=', result.length)
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
      //    if (this.debug) console.log('sendMessage: response=', response)
      
            resolve(response);
          })
          .catch(error => console.err(err))

          return msgs
        }).catch(error => console.err(err))
      } catch (err) {
        if (this.debug) console.log(err);
        reject(false);
      }
    })
  }


  sleepDone() {
    if (this.debug) console.log('sleep done')
  }

  sleep(ms) {
    setTimeout(this.sleepDone, ms)
  }





  sendMessageSync(message) {
    if (this.sendInProgress) {
      throw 'sendMessage: already sending!';
    } else {
      this.sendInProgress = true
    }

    let sendResult = undefined

    this.sendMessageAsyncGood(message)
      .then(result => {
        sendResult = result
      })
      .catch((err) => {
        console.log('sendMessageSync: failed')
      });

    let sleepCalls = 15000
    while (sleepCalls > 0 && sendResult === undefined) {
      this.sleep(1000)
      sleepCalls--
    }

    if (sleepCalls === 0) {
      console.log('sendMessage timed out!')
    }
    this.sendInProgress = false
    return sendResult
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
        if (this.debug) console.log('result is an array')
        if (this.debug) console.log('result length=', result.length)
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

}

module.exports = ZMQCommands

