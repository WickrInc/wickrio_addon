const zmq = require("zeromq")
const { parentPort } = require("node:worker_threads");

class ZMQSendMessage {
  constructor() {

  }

  async sendMessageAsync(args) {
    console.log('entering sendMessageAsync')
  
    const socket = args.socket;
    const message = args.message;
  
    let safeMessage = message.replace(/[\u007f-\uffff]/g,
      function(c) {
        return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
      });
  
    try {
      socket.send(message).then(results => {
        const [msgs] = socket.receive().then((result) => {
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
          
          resolve(response);
        })
        .catch(error => console.err(err))
  
        return msgs
      }).catch(error => console.err(err))
    } catch (err) {
      return {
        success : false,
      }
    }
  }


  init() {
    parentPort.addListener("message", async ({ signal, port, args }) => {
      console.log('parentPort listener')
      // This is the async function that we want to run "synchronously"
      const result = await this.sendMessageAsync(args);
    
      // Post the result to the main thread before unlocking "signal"
      port.postMessage({ result });
      port.close();
    
      // Change the value of signal[0] to 1
      Atomics.store(signal, 0, 1);
    
      // This will unlock the main thread when we notify it
      Atomics.notify(signal, 0);
    });      
  }
}

module.exports = ZMQSendMessage