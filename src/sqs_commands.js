/*
import {
  ReceiveMessageCommand,
  DeleteMessageCommand,
  SQSClient,
  DeleteMessageBatchCommand,
} from "@aws-sdk/client-sqs";
*/

const SQS = require('@aws-sdk/client-sqs')

const deasync = require('deasync')

class SQSCommands {
  constructor(botname, sqsRegion, messageQueue, requestQueue, responseQueue, debugOn) {
    this.botname = botname
    if (sqsRegion)
      this.sqsRegion = sqsRegion
    else
      this.sqsRegion = 'us-east-1'
    this.msgQueue = messageQueue
    this.reqQueue = requestQueue
    this.rspQueue = responseQueue
    this.reqSeqNum = 1                // The sequence number for each request

    this.sendInProgress = false
    this.lastSentMessage = ''
    this.debug = debugOn

    this.client = new SQS.SQSClient({ region: this.sqsRegion });

    const receiveMessage = (queueUrl) =>
      client.send(
        new SQS.ReceiveMessageCommand({
          AttributeNames: ["SentTimestamp"],
          MaxNumberOfMessages: 1,
          MessageAttributeNames: ["All"],
          QueueUrl: this.msgQueue,
          WaitTimeSeconds: 10,
          VisibilityTimeout: 20,
        }),
      );
  }

  async startup() {
  }


  async sendMessage(message) {

    let safeMessage = message.replace(/[\u007f-\uffff]/g,
    function(c) {
      return '\\u'+('0000'+c.charCodeAt(0).toString(16)).slice(-4);
    });

    try {
      if (this.debug) console.log('Sending message:, ',message);

      // Update the sequence number
      this.reqSeqNum++;

      const r = (Math.random() + 1).toString(36).substring(7);
      const command = new SQS.SendMessageCommand({
        QueueUrl: this.reqQueue,
        MessageAttributes: {
          Title: {
            DataType: "String",
            StringValue: "API Request",
          },
          Author: {
            DataType: "String",
            StringValue: this.botname,
          },
          WickrIOSeqNum: {
              DataType: "Number",
              StringValue: this.reqSeqNum.toString(),
          },
        },
        MessageBody: message,
//        MessageGroupId: 'apiRequest',
//        MessageDeduplicationId: r,
      });
    
      const sendRsp = await this.client.send(command);
//      if (this.debug) console.log(sendRsp);
      
      if (this.debug) console.log('message sent, now will receive')

      const result = await this.getMessage(this.rspQueue, this.reqSeqNum)

      // Save the current message just in case
      this.lastSentMessage = message

      if (result === undefined || result.length === 0) {
        console.log('sendMessage: response from engine is undefined or length 0!')

        return( {
          success : false,
          return_code : '',
          result : '',
        });
      }

      let receive_result = ''
      if (Array.isArray(result)) {
        if (this.debug) console.log('result is an array')
        receive_result = result[0];
      } else {
        receive_result = result
      }

      let response = {};

      if (receive_result === undefined) {
        console.log('sendMessage: response from engine is undefined!')
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
        if (this.debug) console.log('sendMessage: response=', response)
        return(response);
      }

      const retVal = receive_msg.substring(0, pos);

      response = {
        success : (retVal === '0'),
        return_code : retVal,
        result : receive_msg.substring(pos+1),
      };
    if (this.debug) console.log('sendMessage: response=', response)

      return(response);
    } catch(err) {
      console.error(err);
      console.error('last message:', this.lastSentMessage )

      return( {
        success : false,
        return_code : '500',
        result : err,
      });
    }
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
              if (this.debug) console.log('sendMessage: response has more than one response!')
            }
      
            if (Array.isArray(result)) {
              if (this.debug) console.log('result is an array')
              if (this.debug) console.log('result lenght=', result.length)
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

  async getMessage(queue, sequenceNumber) {
    try {
      let receiveQueue;
      if (queue) {
        receiveQueue = queue
      } else {
        receiveQueue = this.msgQueue
      }

      let gotMessage = false;
      let loopCount = 10;

      while (!gotMessage) {
        const response = await this.client.send(
          new SQS.ReceiveMessageCommand({
            MaxNumberOfMessages: 1,
            QueueUrl: receiveQueue,
            WaitTimeSeconds: 2,
            VisibilityTimeout: 0,
            MessageAttributeNames: ["All"],
  //          MessageGroupId: 'apiResponse',
          }),
        );

        if (!response?.Messages || response.Messages.length === 0) {
          loopCount--;
          if (loopCount === 0) {
            return(undefined);
          }
        } else {

          const receive_msg = response.Messages[0].Body;

          if (sequenceNumber !== undefined) {
            console.log('attributes:', response.Messages[0].Attributes)
            gotMessage = true;
          } else {
            gotMessage = true;
          }
  
          if (gotMessage) {
            // Send response to finish the send/receive sequence
            const message = 'success'
   
            await this.client.send(
              new SQS.DeleteMessageCommand({
                QueueUrl: receiveQueue,
                ReceiptHandle: response.Messages[0].ReceiptHandle,
              }),
            );
  
            if (receive_msg === undefined) {
              return undefined
            }
    
            return receive_msg
          }
        }
      }

    } catch (err) {
      console.log(err)
      return undefined
    }
  }

}

module.exports = SQSCommands
