var addon = require('bindings')('wickrio_addon');

module.exports = addon;
console.log(addon.display("Hello!"));
console.log(addon.cmdGetStatistics());
