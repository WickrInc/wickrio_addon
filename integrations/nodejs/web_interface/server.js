const express = require('express');
const https = require('https');
const bodyParser = require('body-parser');
const addon = require('wickrio_addon');
const fs = require('fs');
const app = express();
process.title = "wickrioWebApi";
process.stdin.resume(); //so the program will not close instantly
process.setMaxListeners(0);

function exitHandler(options, err) {
  if (err) {
    console.log('Error:', err.stack);
    console.log(addon.closeClient());
    return process.exit();
  }
  if (options.exit) {
    console.log(addon.closeClient());
    return process.exit();
  } else if (options.pid) {
    console.log(addon.closeClient());
    return process.kill(process.pid);
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

var bot_username, bot_port, bot_api_key, bot_api_auth_token, ssl_key_location, ssl_crt_location;

return new Promise(async (resolve, reject) => {
  var client = fs.readFileSync('client_bot_info.txt', 'utf-8');
  client = client.split('\n');
  bot_username = client[0].substring(client[0].indexOf('=') + 1, client[0].length);
  bot_port = client[1].substring(client[1].indexOf('=') + 1, client[1].length);
  bot_api_key = client[2].substring(client[2].indexOf('=') + 1, client[2].length);
  bot_api_auth_token = client[3].substring(client[3].indexOf('=') + 1, client[3].length);
  https_choice = client[4].substring(client[4].indexOf('=') + 1, client[4].length);
  ssl_key_location = client[5].substring(client[5].indexOf('=') + 1, client[5].length);
  ssl_crt_location = client[6].substring(client[6].indexOf('=') + 1, client[6].length);

  // console.log(bot_username, bot_username.length);
  // console.log(bot_port);
  // console.log(bot_api_key);
  // console.log(bot_api_auth_token);
  if (process.argv[2] === undefined) {
    var response = await addon.clientInit(bot_username);
    resolve(response);
  } else {
    var response = await addon.clientInit(process.argv[2]);
    resolve(response);
  }
}).then(result => {
  console.log(result);
  app.use(bodyParser.json());

  if (https_choice === 'yes' || https_choice === 'y') {
    const credentials = {
      key: fs.readFileSync(ssl_key_location, 'utf8'),
      cert: fs.readFileSync(ssl_crt_location, 'utf8')
    };

    https.createServer(credentials, app).listen(bot_port, () => {
      console.log('HTTPS Server running on port', bot_port);
    });
  } else {
    app.listen(bot_port, () => {
      console.log('We are live on ' + bot_port);
    });
  }
  //Basic function to validate credentials for example
  function checkCreds(authToken) {
    var valid = true;
    console.log('authToken pre base64 decode:', authToken);
    const authStr = new Buffer(authToken, 'base64').toString();
    console.log('authToken post base64 decode:',authStr);
    //implement authToken verification in here
    if (authStr !== bot_api_auth_token)
      valid = false;
    return valid;
  }

  var endpoint = "/Apps/" + bot_api_key;

  app.post(endpoint + "/Messages", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    // Check credentials
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      // res.setHeader('WWW-Authenticate', 'Basic realm="example"') //try this out
      return res.send('Access denied: invalid basic-auth token.');
    } else {

      if(!req.body.users && !req.body.vgroupid){
        return res.send("Need a list of users OR a vGroupID to send a message.");
      }
      else if(!req.body.message && !req.body.attachment){
        return res.send("Need a message OR an attachment to send a message.");
      }
      var ttl = "",
        bor = "";
      if (req.body.ttl)
        ttl = req.body.ttl.toString();
      if (req.body.bor)
        bor = req.body.bor.toString();
      if (req.body.users) {
        // var users = req.body.users;
        var users = [];
        for (var i in req.body.users) {
          users.push(req.body.users[i].name);
        }
        if (req.body.attachment) {
          // if(!req.body.attachment.displayname){
          //   return res.send("Attachment displayname must be set.")
          // }
          var attachment;
          var displayName = "";
          if(req.body.attachment.displayname)
            displayName = req.body.attachment.displayname;
          if (req.body.attachment.url) {
            attachment = req.body.attachment.url;
          } else {
            attachment = req.body.attachment.filename;
          }
          console.log('users:', users);
          console.log('attachment:', attachment);
          console.log('displayName:', displayName);
          var s1t1a = addon.cmdSend1to1Attachment(users, attachment, displayName, ttl, bor);
          if (s1t1a === "Sending message") {
            console.log('140:',s1t1a);
            res.send(s1t1a);
          } else {
            res.statusCode = 400;
            res.send(s1t1a);          }
        } else {
          var message = req.body.message;
          console.log("send1to1Message");
          console.log(users, message, ttl, bor);
          var csm = addon.cmdSend1to1Message(users, message, ttl, bor);
          console.log(csm);
          if (csm === "Sending message") {
            res.send(csm);
          } else {
            res.statusCode = 400;
            res.send(csm);
          }
        }
      }
      else if (req.body.vgroupid) {
        var vGroupID = req.body.vgroupid;
        if (req.body.attachment) {

          //ASK if should add this or just convert it to empty string
          // if(!req.body.attachment.displayname){
          //   return res.send("Attachment displayname must be set.")
          // }
          var attachment;
          var displayName = "";
          if(req.body.attachment.displayname)
            displayName = req.body.attachment.displayname;
          if (req.body.attachment.url) {
            attachment = req.body.attachment.url;
          } else {
            attachment = req.body.attachment.filename;
          }
          console.log('attachment:', attachment);
          console.log('displayName:', displayName);
          var csra = await addon.cmdSendRoomAttachment(vGroupID, attachment, displayName, ttl, bor);
          console.log(csra);
          if (csra === "Sending message") {
            res.send(csra);
          } else {
            res.statusCode = 400;
            res.send(csra);
          }
        } else {
          var message = req.body.message;
          var csrm = await addon.cmdSendRoomMessage(vGroupID, message, ttl, bor);
          console.log(csrm);
          if (csrm === "Sending message") {
            res.send(csrm);
          } else {
            res.statusCode = 400;
            res.send(csrm);
          }
        }
      }
    }
  });


  app.get(endpoint + "/Statistics", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var statistics = await addon.cmdGetStatistics();
      console.log(statistics);
      if (statistics !== "") {
        res.send(statistics);
      } else {
        res.statusCode = 400;
        res.send(statistics);
      }
      res.end();
    }
  });


  app.delete(endpoint + "/Statistics", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var statistics = await addon.cmdClearStatistics();
      console.log(statistics);
      if (statistics !== "") {
        res.send("statistics cleared successfully.");
      } else {
        res.statusCode = 400;
        res.send(statistics);
      }
      res.end();
    }
  });


  app.post(endpoint + "/Rooms", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var room = req.body.room;
      console.log("room.masters:",room.masters)
      if(!room.title || !room.description || !room.members || !room.masters){
        return res.send("To Create a secure room you must at least send the following Arguments: Title, description, members and masters.")
      }
      var title = room.title;
      var description = room.description;
      var ttl = "",
        bor = "";
      if (room.ttl)
        ttl = room.ttl.toString();
      if (room.bor)
        bor = room.bor.toString();
      var members = [],
        masters = [];
      for (var i in room.members) {
        members.push(room.members[i].name);
      }

      for (var i in room.masters) {
        masters.push(room.masters[i].name);
      }
      var car = await addon.cmdAddRoom(members, masters, title, description, ttl, bor);
      console.log(car);
      if (car.vgroupid) {
        res.send(car);
      } else {
        res.statusCode = 400;
        res.send(car);
      }
      res.end();
    }
  });

  app.get(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var vGroupID = req.params.vGroupID;
      var cgr = await addon.cmdGetRoom(vGroupID);
      console.log(cgr);
      if (cgr.vgroupid) {
        res.send(cgr);
      } else {
        res.statusCode = 400;
        res.send(cgr);
      }
      res.end();
    }
  });

  app.get(endpoint + "/Rooms", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var cgr = await addon.cmdGetRooms();
      console.log(cgr);
      if (cgr.rooms) {
        res.send(cgr);
      } else {
        res.statusCode = 400;
        res.send(cgr);
      }
      res.end();
    }
  });

  app.delete(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var vGroupID = req.params.vGroupID;
      var reason = req.query.reason;
      if (reason === 'leave') {
        var clr = await addon.cmdLeaveRoom(vGroupID);
        console.log(clr);
        if (clr === "") {
          res.send(bot_username + " left room successfully.");
        } else {
          res.statusCode = 400;
          res.send(clr);
        }
      } else {
        var cdr = await addon.cmdDeleteRoom(vGroupID);
        console.log(cdr);
        if (cdr === "") {
          res.send("Room deleted successfully.");
        } else {
          res.statusCode = 400;
          res.send(cdr);
        }
      }
      res.end();
    }
  });

  //ModifyRoom
  app.post(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var vGroupID = req.params.vGroupID;
      if(typeof vGroupID !== 'string')
        return res.send("vGroupID must be a string.");
      var ttl = "",
        bor = "",
        title = "",
        description = "";
      if (req.body.ttl)
        ttl = req.body.ttl.toString();
      if (req.body.bor)
        bor = req.body.bor.toString();
      if (req.body.title)
        title = req.body.title;
      if (req.body.description)
        description = req.body.description;
      var members = [],
        masters = [];
      if (req.body.members) {
        for (var i in req.body.members) {
          members.push(req.body.members[i].name);
        }
      }
      if (req.body.masters) {
        for (var i in req.body.masters) {
          masters.push(req.body.masters[i].name);
        }
      }
      var cmr = await addon.cmdModifyRoom(vGroupID, members, masters, title, description, ttl, bor);
      console.log(cmr);
      if (cmr === "") {
        res.send("Room modified successfully, unless bot is not a room moderator.");
      } else {
        res.statusCode = 400;
        res.send(cmr);
      }
      res.end();
    }
  });

  app.post(endpoint + "/GroupConvo", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var groupconvo = req.body.groupconvo;
      if(!groupconvo.members)
      return res.send("An array of GroupConvo members is required")
      var ttl = "",
        bor = "";
      if (groupconvo.ttl)
        ttl = groupconvo.ttl.toString();
      if (groupconvo.bor)
        bor = groupconvo.bor.toString();
      var members = [];
      for (var i in groupconvo.members) {
        members.push(groupconvo.members[i].name);
      }
      var cagc = await addon.cmdAddGroupConvo(members, ttl, bor);
      console.log(cagc);
      if (cagc.vgroupid) {
        res.send(cagc);
      } else {
        res.statusCode = 400;
        res.send(cagc);
      }
      res.end();
    }
  });


  app.get(endpoint + "/GroupConvo", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var cggc = await addon.cmdGetGroupConvos();
      console.log(cggc);
      if (cggc.groupconvos) {
        res.send(cggc);
      } else {
        res.statusCode = 400;
        res.send(cggc);
      }
      res.end();
    }
  });


  app.get(endpoint + "/GroupConvo/:vGroupID", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var vGroupID = req.params.vGroupID;
      var cggc = await addon.cmdGetGroupConvo(vGroupID);
      console.log(cggc);
      if (cggc.vgroupid) {
        res.send(cggc);
      } else {
        res.statusCode = 400;
        res.send(cggc);
      }
      res.end();
    }
  });


  app.delete(endpoint + "/GroupConvo/:vGroupID", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var vGroupID = req.params.vGroupID;
      var cdgc = await addon.cmdDeleteGroupConvo(vGroupID);
      console.log(cdgc);
      if (cdgc === "") {
        res.send(bot_username + " has left GroupConvo.");
      } else {
        res.statusCode = 400;
        res.send(cggc);
      }
      res.end();
    }
  });


  app.get(endpoint + "/Messages", async function(req, res) {
    console.log('req.body:', req.body);
    console.log('req.headers:', req.headers);
    var authHeader = req.get('Authorization');
    var authToken;
    if (authHeader) {
      authHeader = authHeader.split(' ');
      authToken = authHeader[1];
    }
    if (!authHeader || !checkCreds(authToken)) {
      res.statusCode = 401;
      return res.end('Access denied: invalid basic-auth token.');
    } else {
      var count = 1;
      var index = 0;
      if (req.query.count)
        count = req.query.count;
      var msgArray = [];
      for (var i=0; i < count; i++) {
          var message = await addon.cmdGetReceivedMessage();
          if (message === "{ }" || message === "" || !message) {
            continue;
          } else {
            msgArray.push(JSON.parse(message));
            console.log(message);
          }
      }
      res.send(msgArray);
      res.end();
    }
  });


  //Finish later: These calls have to be added to the C++ client API first

  // app.post(endpoint + "/MsgRecvCallbackFunction", async function(req, res) {
  // console.log('req.body:', req.body);
  // console.log('req.headers:', req.headers);
  //   var authHeader = req.get('Authorization');
  // var authToken;
  // if (authHeader) {
  //   authHeader = authHeader.split(' ');
  //   authToken = authHeader[1];
  // }
  //   if (!authHeader || !checkCreds(authToken)) {
  //     res.statusCode = 401;
  //     return res.end('Access denied: invalid basic-auth token.');
  //   } else {
  //     var callbackFunction = req.query.callback;
  //     var setMsgURLCallback = addon.cmdStartAsyncRecvMessages(callbackFunction);
  //     if (setMsgURLCallback === undefined)
  //       res.send('SUCCESS');
  //     else {
  //       res.statusCode = 400;
  //       res.send(setMsgURLCallback);
  //     }
  //     res.end();
  //   }
  // });


  // app.get(endpoint + "/MsgRecvCallback", async function(req, res) {
  //   // var callbackURL = await fs.readFile("callbackAddress.txt");
  //   var response = {
  //     "callbackurl": callbackURL
  //   };
  //   res.send(response);
  // });
  //
  // app.delete(endpoint + "/MsgRecvCallback", async function(req, res) {
  //   // var callbackURL = await fs.WriteFile("callbackAddress.txt", "");
  //   res.send('SUCCESS');
  // });

}).catch(error => {
  console.log('Error: ', error);
});
