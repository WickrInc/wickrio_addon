const zmq = require("zeromq")
const { workerData, parentPort } = require("node:worker_threads");

/*
 * Input:
 * workerData.socket
 * workerData.message
 */

const socket = workerData.socket;
const message = workerData.message;
  
parentPort.postMessage({});

let safeMessage = message.replace(/[\u007f-\uffff]/g,
  function(c) {
    return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
  });
  
try {
  socket.send(message).then(results => {
    const [msgs] = socket.receive().then((result) => {
      if (result === undefined || result.length === 0) {
        parentPort.postMessage({
          success : false,
          return_code : '',
          result : '',
        });
      } else {
          
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
          parentPort.postMessage(response);
        } else {
          const receive_msg = receive_result.toString();
  
          // Parse the error code from teh beginning of the response
          const pos = receive_msg.search(':');
          if (pos === -1) {
            response = {
              success : true,
              return_code : '0',
              result : receive_msg,
            };
            parentPort.postMessage(response);
          } else {
            const retVal = receive_msg.substring(0, pos);
          
            response = {
              success : (retVal === '0'),
              return_code : retVal,
              result : receive_msg.substring(pos+1),
            };
          
            parentPort.postMessage(response);
          }
        }
      }
    })
    .catch(error => console.err(err))
  
  }).catch(error => console.err(err))
} catch (err) {
  parentPort.postMessage({
    success : false,
  });
}

