const EventEmitter = require('events')
const ZMQCommands = require('./zmq_commands')
const SQSCommands = require('./sqs_commands')

let clientName='';

async function messages_callback_func(asyncInfo) {
  while (asyncInfo.clientInited === true) {
    const message = await asyncInfo.queueCommands.getMessage();

    if (message !== undefined) {
      if (asyncInfo.messageCallback != undefined)
        asyncInfo.messageCallback(message);
    } else {
      if (asyncInfo.testcount > 0) {
        asyncInfo.testcount--;

        /*
        if (asyncInfo.messageCallback != undefined) {
          asyncInfo.messageCallback('this is a test message!');
          throw 'messages_callback: sent test message!';
        }
        */
      }
    }
  }
}

class WickrIOAddon extends EventEmitter {
  constructor(debugOn) {
    super()
    this.queueCommands = undefined;
    this.messageCallback = undefined;
    this.asyncReceive = false;
    this.clientName = undefined;
    this.clientInited = false;
    this.debug = debugOn

    this.testcount = 2;
  }

  clientInit(username, sqsRegion, sqsMessageQueue, sqsRequestQueue, sqsResponseQueue) {
    this.clientInited = true;

//    throw 'IN clientInit!';

    if (this.debug) console.log('in clientInit')
    this.clientName=username;

    // if the SQS queues are set then use the SQS apis
    if (sqsMessageQueue && sqsRequestQueue && sqsResponseQueue) {
      this.queueCommands = new SQSCommands(username,
                                           sqsRegion,
                                           sqsMessageQueue,
                                           sqsRequestQueue,
                                           sqsResponseQueue,
                                           this.debug);
    } else {
      this.queueCommands = new ZMQCommands(username, this.debug)
    }
    return 'Bot Interface initialized successfully!';
  }

  closeClient() {
    this.clientInited = false;
  }

  async isConnected(delay) {
  /*
    try {
      if (this.debug) console.log('entering isConnected')

      await this.queueCommands.sendMessage('{ "action" : "ping" }').then(results => {
        if (results === undefined || results.result === undefined)
          return false
        return true
      }).catch(err => {
        console.error(err)
        return false
      });
  
      if (this.debug) console.log('leaving isConnected')
    } catch(err) {
      const stack = err.stack
      console.log(stack)
      console.log(err)
    }
    */
    const result = await this.queueCommands.sendMessage('{ "action" : "ping" }');
    if (result === undefined || result.result === undefined)
        return false;
    return true
  }
  
  async getClientState() {
    if (this.debug) console.log('in getClientState')

    const result = await this.queueCommands.sendMessage('{ "action" : "get_state" }');
    return result.result;
  }

  /*
   * Async message receipt functions
   */

  /*
   * continue to pull messages from the message queue while
   * the asyncReceive value is true and there is a callback
   * function
   */
  async messages_callback(asyncInfo) {
    return new Promise(async function(resolve, reject) {
      while (asyncInfo.asyncReceive === true) {
        const message = await asyncInfo.queueCommands.getMessage();

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

  async cmdStartAsyncRecvMessages(callback) {
    this.messageCallback = callback;
    this.asyncReceive = true;

    if (this.debug) console.log('in cmdStartAsyncRecvMessages')

    messages_callback_func(this)
    /*
    await msgCallback.then(
      function(value) { if (this.debug) console.log('async message rx finished'); },
      function(value) { if (this.debug) console.log('async message rx failed'); }
    );
    */

    // Send command to the engine
    const result = await this.queueCommands.sendMessage('{ "action" : "start_async_messages" }');
    return result.result;
  }

  async cmdStopAsyncRecvMessages() {
    this.asyncReceive = false;
    this.messageCallback = undefined;

    if (this.debug) console.log('in cmdStopAsyncRecvMessages')
    return 'Success';
  }

  async cmdSetStreamingOff() {
    if (this.debug) console.log('in cmdSetStreamingOff')
    // Send command to the engine
    const result = await this.queueCommands.sendMessage('{ "action" : "set_streaming", "type" : "off" }');
    return 'Success';
  }

  async cmdSetFileStreaming(dest, baseName, maxSize, attachmentLoc) {
    if (this.debug) console.log('in cmdSetFileStreaming')

    const command = '{ "action" : "set_streaming", "type" : "file", "destination" : "' + dest +
              '",  "base_name" : "' + baseName + '", "max_file_size" : ' + maxSize + 
              ', "attachments" : "' + attachmentLoc + '" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return 'Success';
  }

  async cmdPostEvent(event) {
    if (this.debug) console.log('in cmdPostEvent')

    const command = '{ "action" : "post_event", "event" : "' + event + '" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdSetMsgCallback(callback) {
    if (this.debug) console.log('in cmdSetMsgCallback')

    const command = '{ "action" : "set_urlcallback", "callback" : "' + callback + '" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdGetMsgCallback() {
    if (this.debug) console.log('in cmdGetMsgCallback')

    const command = '{ "action" : "get_urlcallback" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdDeleteMsgCallback() {
    if (this.debug) console.log('in cmdDeleteMsgCallback')

    const command = '{ "action" : "delete_urlcallback" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdSetEventCallback(callback) {
    if (this.debug) console.log('in cmdSetEventCallback')

    const command = '{ "action" : "set_eventurlcallback", "callback" : "' + callback + '" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdGetEventCallback() {
    if (this.debug) console.log('in cmdGetEventCallback')

    const command = '{ "action" : "get_eventurlcallback" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdDeleteEventCallback() {
    if (this.debug) console.log('in cmdDeleteEventCallback')

    const command = '{ "action" : "delete_eventurlcallback" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdGetStatistics() {
    if (this.debug) console.log('in cmdGetStatistics')

    const command = '{ "action" : "get_statistics" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdClearStatistics() {
    if (this.debug) console.log('in cmdClearStatistics')

    const command = '{ "action" : "clear_statistics" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdGetRooms() {
    if (this.debug) console.log('in cmdGetRooms')

    const command = '{ "action" : "get_rooms" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdAddRoom(members, moderators, title, description, ttl, bor) {
    if (this.debug) console.log('in cmdAddRoom')

    if (! Array.isArray(members)) {
      throw 'AddRoom: list of Members must be an array!';
      return 'Failure'
    }
    if (! Array.isArray(moderators)) {
      throw 'AddRoom: list of Moderators must be an array!';
      return 'Failure'
    }

    let commandObj = { 
      action : "add_room",
      room : {
        members : [],
        masters : [],
      }
    };

    for (const member of members) {
      const memberObject = { name : member };
      commandObj.room.members.push(memberObject)
    }
    for (const moderator of moderators) {
      const modObject = { name : moderator };
      commandObj.room.masters.push(modObject)
    }
    if (title?.length > 0) {
      commandObj.room.title = title
    }
    if (description?.length > 0) {
      commandObj.room.description = description
    }
    if (ttl?.length > 0) {
      commandObj.room.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.room.bor = bor
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdModifyRoom(vGroupID, members, moderators, title, description, ttl, bor) {
    if (this.debug) console.log('in cmdModifyRoom')

    if (vGroupID === undefined) {
      throw 'ModifyRoom: vGroupID must be set!';
      return 'Failure'
    }

    if (! Array.isArray(members)) {
      throw 'ModifyRoom: list of Members must be an array!';
      return 'Failure'
    }
    if (! Array.isArray(moderators)) {
      throw 'ModifyRoom: list of Moderators must be an array!';
      return 'Failure'
    }

    let commandObj = { 
      action : "modify_room",
      vgroupid : vGroupID,
      members : [],
      masters : [],
  };

    for (const member of members) {
      const memberObject = { name : member };
      commandObj.members.push(memberObject)
    }
    for (const moderator of moderators) {
      const modObject = { name : moderator };
      commandObj.masters.push(modObject)
    }
    if (title?.length > 0) {
      commandObj.title = title
    }
    if (description?.length > 0) {
      commandObj.description = description
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    
    const command = JSON.stringify(commandObj);
    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetRoom(vGroupID) {
    if (this.debug) console.log('in cmdGetRoom')

    if (vGroupID === undefined) {
      throw 'GetRoom: vGroupID must be set!';
      return 'Failure'
    }
    let commandObj = { 
      action : "get_room",
      vgroupid : vGroupID,
    };

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdLeaveRoom(vGroupID) {
    if (this.debug) console.log('in cmdLeaveRoom')

    if (vGroupID === undefined) {
      throw 'LeaveRoom: vGroupID must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "leave_room",
      vgroupid : vGroupID,
    };

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdDeleteRoom(vGroupID) {
    if (this.debug) console.log('in cmdDeleteRoom')

    if (vGroupID === undefined) {
      throw 'DeleteRoom: vGroupID must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "delete_room",
      vgroupid : vGroupID,
    };

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdAddGroupConvo(members, ttl, bor) {
    if (this.debug) console.log('in cmdAddGroupConvo')

    if (! Array.isArray(members)) {
      throw 'AddGroupConvo: list of Members must be an array!';
      return 'Failure'
    }

    let commandObj = { 
      action : "add_groupconvo",
      groupconvo : {
        members : [],
      }
    };

     for (const member of members) {
      const memberObject = { name : member };
      commandObj.groupconvo.members.push(memberObject)
    }
    if (ttl?.length > 0) {
      commandObj.groupconvo.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.groupconvo.bor = bor
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdDeleteGroupConvo(vGroupID) {
    if (this.debug) console.log('in cmdDeleteGroupConvo')

    if (vGroupID === undefined) {
      throw 'DeleteGroupConvo: vGroupID must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "delete_groupconvo",
      vgroupid : vGroupID,
    };

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdGetGroupConvo(vGroupID) {
    if (this.debug) console.log('in cmdGetGroupConvo')

    if (vGroupID === undefined) {
      throw 'GetGroupConvo: vGroupID must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "get_groupconvo",
      vgroupid : vGroupID,
    };

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }
  
  async cmdGetGroupConvos() {
    if (this.debug) console.log('in cmdGetGroupConvos')

    const command = '{ "action" : "get_groupconvos" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdGetReceivedMessage() {
    if (this.debug) console.log('in cmdGetReceivedMessage')

    const command = '{ "action" : "get_received_messages" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    return result.result;
  }

  async cmdSend1to1Message(users, message, ttl, bor, messageID, flags, messageMetaData, isLowPriority) {
    if (this.debug) console.log('in cmdSend1to1Message')

    if (! Array.isArray(users) || users.length === 0) {
      throw 'Send1to1Message: list of Users must be an array!';
      return 'Failure'
    }
    if (message === undefined || message === null) {
      throw 'Send1to1Message: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined && flags !== null) {
      if (! Array.isArray(flags)) {
        throw 'Send1to1Message: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message",
      message : message,
      users : [],
    };

    for (const user of users) {
      const userObject = { name : user };
      commandObj.users.push(userObject)
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (isLowPriority) {
      commandObj.low_priority = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSend1to1Attachment(users, attachment, displayName, ttl, bor, messageMetaData, deleteWhenSent, isLowPriority) {
    if (this.debug) console.log('in cmdSend1to1Attachment')

    if (! Array.isArray(users)) {
      throw 'Send1to1Attachment: list of Users must be an array!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'Send1to1Attachment: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      users : [],
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    for (const user of users) {
      const userObject = { name : user };
      commandObj.users.push(userObject)
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (isLowPriority) {
      commandObj.low_priority = true
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdSendMessageUserNameFile(userNameFile, message, ttl, bor, messageID, flags, messageMetaData, isLowPriority) {
    if (this.debug) console.log('in cmdSendMessageUserNameFile')

    if (userNameFile === undefined) {
      throw 'SendMessageUserNameFile: User Name file must be set!';
      return 'Failure'
    }
    if (message === undefined) {
      throw 'SendMessageUserNameFile: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined) {
      if (! Array.isArray(flags)) {
        throw 'SendMessageUserNameFile: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message",
      message : message,
      username_filename : userNameFile,
  };

    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (isLowPriority) {
      commandObj.low_priority = true
    }

    const command = JSON.stringify(commandObj);
  
    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendMessageUserHashFile(userHashFile, message, ttl, bor, messageID, flags, messageMetaData, isLowPriority) {
    if (this.debug) console.log('in cmdSendMessageUserHashFile')

    if (userHashFile === undefined) {
      throw 'SendMessageUserHashFile: User Hash file must be set!';
      return 'Failure'
    }
    if (message === undefined) {
      throw 'SendMessageUserHashFile: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined) {
      if (! Array.isArray(flags)) {
        throw 'SendMessageUserHashFile: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message",
      message : message,
      userid_filename : userHashFile,
  };

    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (isLowPriority) {
      commandObj.low_priority = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendAttachmentUserNameFile(userNameFile, attachment, displayName, ttl, bor, messageID, messageMetaData, message, deleteWhenSent) {
    if (this.debug) console.log('in cmdSendAttachmentUserNameFile')

    if (userNameFile === undefined) {
      throw 'SendMessageUserNameFile: User Name file must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendMessageUserNameFile: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      username_filename : userNameFile,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);
   
    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendAttachmentUserHashFile(userIDFile, attachment, displayName, ttl, bor, messageID, messageMetaData, message, deleteWhenSent) {
    if (this.debug) console.log('in cmdSendAttachmentUserHashFile')

    if (userIDFile === undefined) {
      throw 'SendMessageUserHashFile: User Hash file must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendMessageUserHashFile: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      userid_filename : userIDFile,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendVoiceMemoUserNameFile(userNameFile, attachment, displayName, duration, ttl, bor, messageID, messageMetaData, message) {
    if (this.debug) console.log('in cmdSendVoiceMemoUserNameFile')

    if (userNameFile === undefined) {
      throw 'SendMessageUserNameFile: User Name file must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendMessageUserNameFile: attachment must be set!';
      return 'Failure'
    }
    if (duration?.length === 0 || !duration.isNumber()) {
      throw 'SendMessageUserNameFile: duration must be set as a number!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      userid_filename : userIDFile,
      isvoicememo : true,
      duration : duration,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendVoiceMemoUserHashFile(userIDFile, attachment, displayName, duration, ttl, bor, messageID, messageMetaData, message) {
    if (this.debug) console.log('in cmdSendVoiceMemoUserHashFile')

    if (userIDFile === undefined) {
      throw 'SendMessageUserHashFile: User Id file must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendMessageUserHashFile: attachment must be set!';
      return 'Failure'
    }
    if (duration?.length === 0 || !duration.isNumber()) {
      throw 'SendMessageUserHashFile: duration must be set as a number!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      userid_filename : userIDFile,
      isvoicememo : true,
      duration : duration,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdSendRoomMessage(vGroupID, message, ttl, bor, messageID, flags, messageMetaData) {
    if (this.debug) console.log('in cmdSendRoomMessage')

    if (vGroupID === undefined || vGroupID === null || vGroupID.length === 0) {
      throw 'SendRoomMessage: vGroupID must be set!';
      return 'Failure'
    }
    if (message === undefined) {
      throw 'SendRoomMessage: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined && flags !== null) {
      if (! Array.isArray(flags)) {
        throw 'SendRoomMessage: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message",
      message : message,
      vgroupid : vGroupID,
    };

    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }

    const command = JSON.stringify(commandObj);
   
    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendRoomAttachment(vGroupID, attachment, displayName, ttl, bor, messageMetaData, deleteWhenSent) {
    if (this.debug) console.log('in cmdSendRoomAttachment')

    if (vGroupID === undefined || vGroupID.length === 0) {
      throw 'SendRoomAttachment: vGroupID must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendRoomAttachment: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message",
      attachment : {},
      vgroupid : vGroupID,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'Send1to1Attachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendNetworkMessage(message, ttl, bor, messageID, flags, messageMetaData) {
    if (this.debug) console.log('in cmdSendNetworkMessage')

    if (message === undefined || message === null) {
      throw 'SendNetworkMessage: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined && flags !== null) {
      if (! Array.isArray(flags)) {
        throw 'SendNetworkMessage: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message_network",
      message : message,
    };

    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendSecurityGroupMessage(message, securityGroups, ttl, bor, messageID, flags, messageMetaData) {
    if (this.debug) console.log('in cmdSendSecurityGroupMessage')

    if (securityGroups === undefined || ! Array.isArray(securityGroups) || securityGroups.length === 0) {
      throw 'SendSecurityGroupMessage: security groups must be set!';
      return 'Failure'
    }
    if (message === undefined) {
      throw 'SendSecurityGroupMessage: message must be set!';
      return 'Failure'
    }
    if (flags !== undefined) {
      if (! Array.isArray(flags)) {
        throw 'SendSecurityGroupMessage: flags must be an array if set!';
        return 'Failure'
      }
    }

    let commandObj = { 
      action : "send_message_network",
      message : message,
      security_groups : [],
    };

    for (const securityGroup of securityGroups) {
      commandObj.security_groups.push(securityGroup)
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (flags?.length > 0) {
      commandObj.flags = []
      for (const flag of flags) {
        commandObj.flags.push(flag)
      }
    }
    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdSendNetworkAttachment(attachment, displayName, ttl, bor, messageID, message, messageMetaData, deleteWhenSent) {
    if (this.debug) console.log('in cmdSendNetworkAttachment')

    if (attachment === undefined) {
      throw 'SendRoomAttachment: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message_network",
      attachment : {},
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'SendRoomAttachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);
  
    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendSecurityGroupAttachment(securityGroups, attachment, displayName, ttl, bor, messageID, message, messageMetaData, deleteWhenSent) {
    if (this.debug) console.log('in cmdSendSecurityGroupAttachment')

    if (securityGroups === undefined || ! Array.isArray(securityGroups) || securityGroups.length === 0) {
      throw 'SendSecurityGroupAttachment: security groups must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendSecurityGroupAttachment: attachment must be set!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message_network",
      attachment : {},
      security_groups : [],
    };

    for (const securityGroup of securityGroups) {
      commandObj.security_groups.push(securityGroup)
    }

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'SendSecurityGroupAttachment: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }
    if (deleteWhenSent) {
      commandObj.deletewhensent = true
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdSendNetworkVoiceMemo(attachment, displayName, duration, ttl, bor, messageID, message, messageMetaData) {
    if (this.debug) console.log('in cmdSendNetworkVoiceMemo')

    if (attachment === undefined) {
      throw 'SendNetworkVoiceMemo: attachment must be set!';
      return 'Failure'
    }
    if (duration?.length === 0 || !(typeof duration == 'string')) {
      throw 'SendNetworkVoiceMemo: duration must be String!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message_network",
      attachment : {},
      isvoicememo : true,
      duration : duration,
    };

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'SendNetworkVoiceMemo: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdSendSecurityGroupVoiceMemo(securityGroups, attachment, displayName, duration, ttl, bor, messageID, message, messageMetaData) {
    if (this.debug) console.log('in cmdSendSecurityGroupVoiceMemo')

    if (securityGroups === undefined || ! Array.isArray(securityGroups) || securityGroups.length === 0) {
      throw 'SendSecurityGroupVoiceMemo: security groups must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendSecurityGroupVoiceMemo: attachment must be set!';
      return 'Failure'
    }
    if (duration?.length === 0 || !duration.isNumber()) {
      throw 'SendSecurityGroupVoiceMemo: duration must be set as a number!';
      return 'Failure'
    }

    let commandObj = { 
      action : "send_message_network",
      attachment : {},
      isvoicememo : true,
      duration : duration,
      security_groups : [],
    };

    for (const securityGroup of securityGroups) {
      commandObj.security_groups.push(securityGroup)
    }

    // If this starts with HTTP/http we treat it as a URL
    if (attachment.toLowerCase().startsWith('http')) {
      if (displayName === undefined || displayName.length === 0) {
        throw 'SendSecurityGroupVoiceMemo: display name must be set for url attachments!';
        return 'Failure'
      }
      commandObj.attachment.url = attachment
    } else {
      commandObj.attachment.filename = attachment
    }

    if (messageMetaData && messageMetaData.length > 0) {
      const messageMetaObject = JSON.parse(messageMetaData);
      commandObj.messagemeta = messageMetaObject
    }
    if (displayName?.length > 0) {
      commandObj.attachment.displayname = displayName
    }
    if (ttl?.length > 0) {
      commandObj.ttl = ttl
    }
    if (bor?.length > 0) {
      commandObj.bor = bor
    }
    if (messageID?.length > 0) {
      commandObj.message_id = messageID
    }
    if (message?.length > 0) {
      commandObj.message = message
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdEncryptString(value) {
    if (this.debug) console.log('in cmdEncryptString')

    const commandObj = {
      action : 'encrypt_string',
      string : value,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdDecryptString(value) {
    if (this.debug) console.log('in cmdDecryptString')

    const commandObj = {
      action : 'decrypt_string',
      string : value,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdAddKeyValue(key, value) {
    if (this.debug) console.log('in cmdAddKeyValue')

    const commandObj = {
      action : 'add_keyvalue',
      key : key,
      value : value,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }
  
  async cmdGetKeyValue(key) {
    if (this.debug) console.log('in cmdGetKeyValue')

    const commandObj = {
      action : 'get_keyvalue',
      key : key,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdDeleteKeyValue(key) {
    if (this.debug) console.log('in cmdDeleteKeyValue')

    const commandObj = {
      action : 'delete_keyvalue',
      key : key,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdClearAllKeyValues() {
    if (this.debug) console.log('in cmdClearAllKeyValues')

    const commandObj = {
      action : 'clearall_keyvalues',
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdSetControl(controlKey, controlValue) {
    if (this.debug) console.log('in cmdSetControl')

    const command = '{ "action" : "set_control", "' + controlKey + '" : "' + controlValue + '" }';

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return 'Success';
    else
      return 'Failure';
  }

  async cmdGetNetworkRooms(network) {
    if (this.debug) console.log('in cmdGetNetworkRooms')

    const commandObj = {
      action : 'get_network_rooms',
      network : network,
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdAddNetworkRoom(network, securityGroups, title, description, ttl, bor) {
    if (this.debug) console.log('in cmdAddNetworkRoom')

    if (attachment === undefined) {
      throw 'SendSecurityGroupVoiceMemo: attachment must be set!';
      return 'Failure'
    }
    if (attachment === undefined) {
      throw 'SendSecurityGroupVoiceMemo: attachment must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'add_network_room',
      room : {
        network : network,
      },
    }

    if (title !== undefined) {
      commandObj.room.title = title
    }
    if (description !== undefined) {
      commandObj.room.description = description
    }
    if (ttl !== undefined) {
      commandObj.room.ttl = ttl
    }
    if (ttborl !== undefined) {
      commandObj.room.bor = bor
    }

    if (securityGroups !== undefined || Array.isArray(securityGroups) || securityGroups.length > 0) {
      commandObj.room.security_groups = []
      for (const securityGroup of securityGroups) {
        commandObj.room.security_groups.push(securityGroup)
      }
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdGetDirectory(page, size) {
    if (this.debug) console.log('in cmdGetDirectory')

    if (page === undefined && size !== undefined) {
      throw 'GetDirectory: page is undefined and size is defined!';
      return 'Failure'
    }
    if (page !== undefined && size === undefined) {
      throw 'GetDirectory: page is defined and size is undefined!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_directory',
    }

    if (page !== undefined) {
      if (typeof page === 'number') {
        commandObj.page = page
      } else {
        commandObj.page = Number(page)
      }
      if (typeof size === 'number') {
        commandObj.size = size
      } else {
        commandObj.size = Number(size)
      }
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success) {
      return result.result;
    } else {
      return 'Failure';
    }
  }
  
  async cmdGetSecurityGroups(page, size) {
    if (this.debug) console.log('in cmdGetSecurityGroups')

    if (page === undefined && size !== undefined) {
      throw 'GetSecurityGroups: page is undefined and size is defined!';
      return 'Failure'
    }
    if (page !== undefined && size === undefined) {
      throw 'GetSecurityGroups: page is defined and size is undefined!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_security_groups',
    }

    if (page !== undefined) {
      if (typeof page === 'number') {
        commandObj.page = page
      } else {
        commandObj.page = Number(page)
      }
      if (typeof size === 'number') {
        commandObj.size = size
      } else {
        commandObj.size = Number(size)
      }
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      {
      return result.result;
      }
    else
      return 'Failure';
  }
  
  async cmdGetSecurityGroupDirectory(securityGroup, page, size) {
    if (this.debug) console.log('in cmdGetSecurityGroupDirectory')

    if (securityGroup === undefined) {
      throw 'GetSecurityGroupDirectory: security group must be entered!';
      return 'Failure'
    }
    if (page === undefined && size !== undefined) {
      throw 'GetSecurityGroupDirectory: page is undefined and size is defined!';
      return 'Failure'
    }
    if (page !== undefined && size === undefined) {
      throw 'GetSecurityGroupDirectory: page is defined and size is undefined!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_directory',
      security_group : securityGroup,
    }

    if (page !== undefined) {
      if (typeof page === 'number') {
        commandObj.page = page
      } else {
        commandObj.page = Number(page)
      }
      if (typeof size === 'number') {
        commandObj.size = size
      } else {
        commandObj.size = Number(size)
      }
    }
    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetMessageStatus(messageID, type, page, size, filter, usersFilter) {
    if (this.debug) console.log('in cmdGetMessageStatus')

    if (messageID === undefined) {
      throw 'GetMessageStatus: Message ID must be entered!';
      return 'Failure'
    }
    if (type !== undefined && (type !== 'full' && type !== 'summary')) {
      throw 'GetMessageStatus: type must be either full or summary!';
      return 'Failure'
    }
    if (page === undefined && size !== undefined) {
      throw 'GetMessageStatus: page is undefined and size is defined!';
      return 'Failure'
    }
    if (page !== undefined && size === undefined) {
      throw 'GetMessageStatus: page is defined and size is undefined!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_message_status',
      message_id : messageID,
    }

    if (type === undefined || type === 'summary') {
      commandObj.type = 'summary'
      if (page !== undefined) {
        if (typeof page === 'number') {
          commandObj.page = page
        } else {
          commandObj.page = Number(page)
        }
        if (typeof size === 'number') {
          commandObj.size = size
        } else {
          commandObj.size = Number(size)
        }
      }
    } else {
      commandObj.type = 'full'
    }

    if (filter?.length > 0) {
      commandObj.filter = filter
    }
    if (usersFilter?.length > 0) {
      commandObj.filter_users = usersFilter
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdSetMessageStatus(messageID, user, status, statusMessage) {
    if (this.debug) console.log('in cmdSetMessageStatus')

    if (messageID === undefined) {
      throw 'SetMessageStatus: Message ID must be entered!';
      return 'Failure'
    }
    if (user === undefined || user.length === 0) {
      throw 'SetMessageStatus: user must be set!';
      return 'Failure'
    }
    if (status === undefined || status.length === 0) {
      throw 'SetMessageStatus: status must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'set_message_status',
      message_id : messageID,
      user : user,
      status : parseInt(status),
    }

    if (statusMessage?.length > 0) {
      commandObj.status_message = statusMessage
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdAddMessageID(messageID, sender, target, dateSent, message) {
    if (this.debug) console.log('in cmdAddMessageID')

    if (messageID === undefined || messageID.length === 0) {
      throw 'AddMessageID: Message ID must be entered!';
      return 'Failure'
    }
    if (sender === undefined || sender.length === 0) {
      throw 'AddMessageID: sender must be set!';
      return 'Failure'
    }
    if (target === undefined || target.length === 0) {
      throw 'AddMessageID: target must be set!';
      return 'Failure'
    }
    if (dateSent === undefined || dateSent.length === 0) {
      throw 'AddMessageID: date sent must be set!';
      return 'Failure'
    }
    if (message === undefined || message.length === 0) {
      throw 'AddMessageID: message must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'add_message_id',
      message_id : messageID,
      sender : sender,
      target : target,
      when_sent : dateSent,
      message : message,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdDeleteMessageID(messageID) {
    if (this.debug) console.log('in cmdDeleteMessageID')

    if (messageID === undefined || messageID.length === 0) {
      throw 'DeleteMessageID: Message ID must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'delete_message_id',
      message_id : messageID,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetMessageIDEntry(messageID) {
    if (this.debug) console.log('in cmdGetMessageIDEntry')

    if (messageID === undefined || messageID.length === 0) {
      throw 'GetMessageIDEntry: Message ID must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_message_id_entry',
      message_id : messageID,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetMessageIDTable(page, size, sender) {
    if (this.debug) console.log('in cmdGetMessageIDTable')

    if (page === undefined || page.length === 0) {
      throw 'GetMessageIDTable: page must be entered!';
      return 'Failure'
    }
    if (size === undefined || size.length === 0) {
      throw 'GetMessageIDTable: size must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_message_id_table',
    }

    if (typeof page === 'number') {
      commandObj.page = page
    } else {
      commandObj.page = Number(page)
    }
    if (typeof size === 'number') {
      commandObj.size = size
    } else {
      commandObj.size = Number(size)
    }

    if (sender?.length > 0) {
      commandObj.sender = sender
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdCancelMessageID(messageID) {
    if (this.debug) console.log('in cmdCancelMessageID')

    if (messageID === undefined || messageID.length === 0) {
      throw 'CancelMessageID: Message ID must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'cancel_message_id',
      message_id : messageID,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdSendDeleteMessage(vGroupID, msgID) {
    if (this.debug) console.log('in cmdSendDeleteMessage')

    if (vGroupID === undefined || vGroupID.length === 0) {
      throw 'SendDeleteMessage: vGroupID must be entered!';
      return 'Failure'
    }
    if (msgID === undefined || msgID.length === 0) {
      throw 'SendDeleteMessage: msgID must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'delete_message',
      vgroupid : vGroupID,
      message_id : msgID,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdSendRecallMessage(vGroupID, msgID) {
    if (this.debug) console.log('in cmdSendRecallMessage')

    if (vGroupID === undefined || vGroupID.length === 0) {
      throw 'SendRecallMessage: vGroupID must be entered!';
      return 'Failure'
    }
    if (msgID === undefined || msgID.length === 0) {
      throw 'SendRecallMessage: msgID must be entered!';
      return 'Failure'
    }

    const commandObj = {
      action : 'delete_message',
      vgroupid : vGroupID,
      message_id : msgID,
      do_recall : true,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdGetVerificationList(mode) {
    if (this.debug) console.log('in cmdGetVerificationList')

    if (mode?.length > 0 && mode !== 'all') {
      throw 'GetVerificationList: mode must be all!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_verfication_list',
    }

    if (mode?.length && mode === 'all') {
      commandObj.mode = 'all'
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdVerifyUsers(users) {
    if (this.debug) console.log('in cmdVerifyUsers')

    if (! Array.isArray(users)) {
      throw 'VerifyUsers: list of Users must be an array!';
      return 'Failure'
    }
    if (users.length === 0) {
      throw 'VerifyUsers: users must contain at least one entry!';
      return 'Failure'
    }

    const commandObj = {
      action : 'verify_user',
      users : [],
    }

    for (const user of users) {
      commandObj.users.push(user)
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdVerifyAll() {
    if (this.debug) console.log('in cmdVerifyAll')

    // Send command to the engine
    const result = await this.queueCommands.sendMessage('{ "action" : "verify_all" }');
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdSetVerificationMode(mode) {
    if (this.debug) console.log('in cmdSetVerificationMode')

    if (mode === undefined || mode.length === 0) {
      throw 'SetVerificationMode: mode must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'set_verification_mode',
      mode : mode,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdGetUserInfo(users) {
    if (this.debug) console.log('in cmdGetUserInfo')

    if (! Array.isArray(users)) {
      throw 'GetUserInfo: list of Users must be an array!';
      return 'Failure'
    }
    if (users.length === 0) {
      throw 'GetUserInfo: users must contain at least one entry!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_user_info',
      users : [],
    }

    for (const user of users) {
      commandObj.users.push(user)
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetServerInfo() {
    if (this.debug) console.log('in cmdGetServerInfo')

    // Send command to the engine
    const result = await this.queueCommands.sendMessage('{ "action" : "get_server_info" }');
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdGetClientInfo() {
    if (this.debug) console.log('in cmdGetClientInfo')
    return { name : this.clientName };
  }

  async cmdSetAvatar(fileName) {
    if (this.debug) console.log('in cmdSetAvatar')

    if (fileName === undefined || fileName.length === 0) {
      throw 'SetAvatar: Avatar file name must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'set_avatar',
      filename : fileName,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
  
  async cmdGetBotsList(networkID, tokenCredentials) {
    if (this.debug) console.log('in cmdGetBotsList')

    if (networkID === undefined || networkID.length === 0) {
      throw 'GetBotsList: networkID must be set!';
      return 'Failure'
    }
    if (tokenCredentials === undefined || tokenCredentials.length === 0) {
      throw 'GetBotsList: tokenCredentials must be set!';
      return 'Failure'
    }

    const commandObj = {
      action : 'get_bots_list',
      token_creds : tokenCredentials,
      network_id : networkID,
    }

    const command = JSON.stringify(commandObj);

    // Send command to the engine
    const result = await this.queueCommands.sendMessage(command);
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }

  async cmdGetTransmitQueueInfo() {
    if (this.debug) console.log('in cmdGetTrnasmitQueueInfo')

    // Send command to the engine
    const result = await this.queueCommands.sendMessage('{ "action" : "get_transmit_queue_info" }');
    if (result.success)
      return result.result;
    else
      return 'Failure';
  }
}

module.exports = {
  WickrIOAddon,
}
