const express = require('express');
const bodyParser = require('body-parser');
const addon = require('wickrio_addon');
const fs = require('fs');
const app = express();

const port = 8090;
var api_key = process.env.API_KEY;

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
  app.use(bodyParser.json());

  app.listen(port, () => {
    console.log('We are live on ' + port);
  });

  var endpoint = "/Apps/" + api_key;

  app.post(endpoint + "/Messages", async function(req, res) {
    // console.log('req.body:', req.body);
    var ttl = "",
      bor = "";
    if (req.body.ttl)
      ttl = req.body.ttl.toString();
    if (req.body.bor)
      bor = req.body.bor.toString();
    if (req.body.users) {
      var users = [];
      for (var i in req.body.users) {
        users.push(req.body.users[i].name);
      }
      if (req.body.attachment) {
        var attachment;
        var displayName = "";
        if (req.body.attachment.url) {
          attachment = req.body.attachment.url.toString();
          displayName = req.body.attachment.displayname.toString();
        } else {
          attachment = req.body.attachment.filename.toString();
          displayName = req.body.attachment.displayname.toString();
        }
        console.log('users:', users);
        console.log('attachment:', attachment);
        console.log('displayName:', displayName);
        var s1t1a = addon.cmdSend1to1Attachment(users, attachment, displayName, ttl, bor);
        if (s1t1a !== "")
          res.send(s1t1a);
        else {
          res.sendStatus(400);
        }
      } else {
        var message = req.body.message;
        console.log("send1to1Message");
        console.log(users, message, ttl, bor);
        var csm = addon.cmdSend1to1Message(users, message, ttl, bor);
        console.log('csm', csm);
        if (csm !== "")
          res.send(csm);
        else {
          res.sendStatus(400);
        }
      }
    } else if (req.body.vgroupid) {
      var vGroupID = req.body.vgroupid.toString();
      if (req.body.attachment) {
        var attachment;
        var displayName = "";
        if (req.body.attachment.url) {
          attachment = req.body.attachment.url.toString();
          displayName = req.body.attachment.displayname.toString();
        } else {
          attachment = req.body.attachment.filename.toString();
          displayName = req.body.attachment.displayname.toString();
        }
        console.log('attachment:', attachment);
        console.log('displayName:', displayName);
        var csra = await addon.cmdSendRoomAttachment(vGroupID, attachment, displayName, ttl, bor);
        if (csra !== "")
          res.send(csra);
        else {
          res.sendStatus(400);
        }
      } else {
        var message = req.body.message;
        var csrm = await addon.cmdSendRoomMessage(vGroupID, message, ttl, bor);
        if (csrm !== "")
          res.send(csrm);
        else {
          res.sendStatus(400);
        }
      }
    }
  });


  app.get(endpoint + "/Statistics", async function(req, res) {
    var statistics = await addon.cmdGetStatistics();
    if (statistics !== "")
      res.send(statistics);
    else {
      res.sendStatus(400);
    }
  });


  app.delete(endpoint + "/Statistics", async function(req, res) {
    var statistics = await addon.cmdClearStatistics();
    if (statistics !== "")
      res.send(statistics);
    else {
      res.sendStatus(400);
    }
    // res.send("statistics cleared successfully");
  });


  app.post(endpoint + "/Rooms", async function(req, res) {
    var room = req.body.room;
    var title = room.title.toString();
    var description = room.description.toString();
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
    if (car !== "")
      res.send(car);
    else {
      res.sendStatus(400);
    }
  });

  app.get(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    var vGroupID = req.params.vGroupID;
    var cgr = await addon.cmdGetRoom(vGroupID);
    if (cgr !== "")
      res.send(cgr);
    else {
      res.sendStatus(400);
    }
  });

  app.get(endpoint + "/Rooms", async function(req, res) {
    var cgr = await addon.cmdGetRooms();
    if (cgr !== "")
      res.send(cgr);
    else {
      res.sendStatus(400);
    }
  });

  app.delete(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    var vGroupID = req.params.vGroupID;
    var reason = req.query.reason;
    if (reason === 'leave') {
      var clr = await addon.cmdLeaveRoom(vGroupID);
      if (clr !== "")
        res.send(clr);
      else
        res.sendStatus(400);
    } else {
      var cdr = await addon.cmdDeleteRoom(vGroupID)
      if (cdr !== "")
        res.send(cdr);
      else
        res.sendStatus(400);
    }
  });

  //ModifyRoom
  app.post(endpoint + "/Rooms/:vGroupID", async function(req, res) {
    var vGroupID = req.params.vGroupID;
    var ttl = "",
      bor = "",
      title = "",
      description = "";
    if (req.body.ttl)
      ttl = req.body.ttl.toString();
    if (req.body.bor)
      bor = req.body.bor.toString();
    if (req.body.title)
      title = req.body.title.toString();
    if (req.body.description)
      description = req.body.description.toString();
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
    if (cmr !== "")
      res.send(cmr);
    else {
      res.sendStatus(400);
    }
  });

  app.post(endpoint + "/GroupConvo", async function(req, res) {
    var groupconvo = req.body.groupconvo;
    var ttl = "",
      bor = "";
    if (groupconvo.ttl)
      ttl = groupconvo.ttl.toString();
    if (groupconvo.bor)
      bor = groupconvo.bor.toString();
    var members = [],
      masters = [];
    for (var i in groupconvo.members) {
      members.push(groupconvo.members[i].name);
    }
    var car = await addon.cmdAddGroupConvo(members, ttl, bor);
    console.log(car);
    if (car !== "")
      res.send(car);
    else {
      res.sendStatus(400);
    }
  });

  app.get(endpoint + "/GroupConvo", async function(req, res) {
    var cggc = await addon.cmdGetGroupConvos();
    if (cggc !== "")
      res.send(cggc);
    else {
      res.sendStatus(400);
    }
  });


  app.get(endpoint + "/GroupConvo/:vGroupID", async function(req, res) {
    var vGroupID = req.params.vGroupID;
    var cggc = await addon.cmdGetGroupConvo(vGroupID);
    if (cggc !== "")
      res.send(cggc);
    else {
      res.sendStatus(400);
    }
  });


  app.delete(endpoint + "/GroupConvo/:vGroupID", async function(req, res) {
    var vGroupID = req.params.vGroupID;
    var cdgc = await addon.cmdDeleteGroupConvo(vGroupID);
    if (cdgc !== "")
      res.send(cdr);
    else
      res.sendStatus(400);
  });

  app.get(endpoint + "/Messages", async function(req, res) {
    var count = 1;
    var index = 0;
    if (req.query.count)
      count = req.query.count;
    var msgArray = [];
    for (var i in count) {
      console.log('count:', count)
      for (;;) {
        var message = addon.cmdGetReceivedMessage();
        if (message === "{ }" || message === "" || !message) {
          index++;
          continue;
        } else {
          index++;
          msgArray.push(JSON.parse(message));
          console.log(message);
          if (index >= count)
            break;
        }
      }
    }
    console.log('msgArray.length:', msgArray.length);
    res.send(msgArray);
  });


  //Finish later
  // app.post(endpoint + "/MsgRecvCallback", async function(req, res) {
  //   var callbackURL = req.query.callbackurl;
  //   var SetMsgURLCallback = addon.cmdSetMsgURLCallback(callbackURL);
  //   // var wfs = await fs.writeFileSync("callbackAddress.txt", callbackURL);
  //   res.send('SUCCESS');
  // });
  //
  // app.get(endpoint + "/MsgRecvCallback", async function(req, res) {
  //   // var callbackURL = await fs.readFileSync("callbackAddress.txt");
  //   var response = {
  //     "callbackurl": callbackURL
  //   };
  //   res.send(response);
  // });
  //
  // app.delete(endpoint + "/MsgRecvCallback", async function(req, res) {
  //   // var callbackURL = await fs.WriteFileSync("callbackAddress.txt", "");
  //   res.send('SUCCESS');
  // });

}).catch(error => {
  console.log('Error: ', error);
});
