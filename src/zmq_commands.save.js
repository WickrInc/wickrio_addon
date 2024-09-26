
const zmq = require("zeromq");
const {
    Worker,
    receiveMessageOnPort,
    isMainThread,
    MessageChannel,
} = require("node:worker_threads");
const ZMQSendMessage = require("./zmq_sendmessage")

/*const {
  ReceiveMessageCommand,
  DeleteMessageCommand,
  SendMessageCommand,
  SQSClient,
  DeleteMessageBatchCommand,
} = require('@aws-sdk/client-sqs')
*/

class ZMQCommands {
  constructor(botname) {
    this.botname = botname
    this.requestQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/0';
    this.asyncQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/2';
    this.heartbeatQName = 'ipc:///opt/WickrIO/clients/' + botname + '/tmp/1';

    this.requestSocket = new zmq.Request
    this.requestSocket.sendTimeout = 5000;
    this.requestSocket.connect(this.requestQName)
    console.log('ZMQCommands: request socket bound to', this.requestQName);

    this.asyncSocket = new zmq.Reply
    //this.asyncSocket.receiveTimeout = 10000;
    this.asyncSocket.receiveTimeout = -1;
    this.asyncSocket.connect(this.asyncQName)
    console.log('ZMQCommands: async socket bound to', this.asyncQName);

    this.zmq_sendmessage = new ZMQSendMessage();


    if (isMainThread) {
      this.worker = new Worker("./zmq_sendmessage.js");
      this.workerSignal = new Int32Array(new SharedArrayBuffer(4));
    } else {
      this.zmq_sendmessage.init()
    }



    this.sendMessageAsync = (message) => 
      new Promise((resolve, reject) => {

        let safeMessage = message.replace(/[\u007f-\uffff]/g,
        function(c) {
          return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
        });

        try {
          this.requestSocket.send(message).then(results => {
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

  sendMessage(message) {
	  /*
    return (result) =>
      this.sendMessageAsync(message).then(results => {
        return results
      })
      */
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
  }

  async getMessage() {
    return undefined;
  }

}

module.exports = ZMQCommands

