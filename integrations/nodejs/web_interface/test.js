const express        = require('express');
const MongoClient    = require('mongodb').MongoClient;
const bodyParser     = require('body-parser');
const app            = express();


app.get("/test", function(req, res){
  console.log(req.body);
  console.log("YAAAAH");
  res.send("OK 200");
});

app.listen(8000);
