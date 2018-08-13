const express        = require('express');
const MongoClient    = require('mongodb').MongoClient;
const bodyParser     = require('body-parser');
const addon          = require('wickrio_addon');
const app            = express();

const port = 8000;
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


app.listen(port, () => {
  console.log('We are live on ' + port);
});

app.post("/Apps/"+ api_key + "/Messages", function(req, res){
  console.log(req.body);
  var message = req.body.message;
  var ttl, bor;
  if(req.body.ttl)
    ttl = req.body.ttl;
  if(req.body.bor)
    bor = req.body.bor;
  if(req.body.users){
  var users = [];
  users = req.body.users;

  if(req.body.attachment){
   var attachment = req.body.attachment;
   var displayName = "";
   if(req.body.attachment.url){
    displayName = req.body.attachment.filename;
   }
   console.log(addon.cmdSend1to1Attachment(members, attachment, displayName, ttl, bor));
   }
   else{
   var csm = addon.cmdSend1to1Message(users, message, ttl , bor);
   console.log(csm);
   }
}
  else if(req.body.vgroupid){
  var vGroupID = req.body.vgroupid;
  if(req.body.attachment){
   var attachment = req.body.attachment;
   var displayName = "";
   if(req.body.attachment.url){
    displayName = req.body.attachment.filename;
   }
   console.log(addon.cmdSendRoomAttachment(vGroupID, attachment, displayName, ttl, bor));
   }
   else{
     console.log(addon.cmdSendRoomMessage(vGroupID, message, ttl, bor));
   }
  }

});
}).catch(error => {
  console.log('Error: ', error);
});
