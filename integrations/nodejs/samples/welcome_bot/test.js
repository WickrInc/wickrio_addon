// test.js
var addon = require('wickrio_addon');

addon(printer);

function printer(message){
  console.log('message:', message);
}
