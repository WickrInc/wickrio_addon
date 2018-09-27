var addon = require('wickrio_addon');
var fs = require('fs');

process.title = "helloWorldBot";
module.exports = addon;
process.stdin.resume(); //so the program will not close instantly

function exitHandler(options, err) {
  if (err) {
    console.log('Error:',err.stack);
    console.log(addon.closeClient());
    process.exit();
  }
  if (options.exit) {
    console.log(addon.closeClient());
    process.exit();
  } else if (options.pid) {
    console.log(addon.closeClient());
    process.kill(process.pid);
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

  var responseMessageList = [
    "Hey there! Thanks for messaging me! I have a few helpful but random tips I can share in response to your messages, " +
    "so please bear with me☺ If you have more questions than I have answers, head to Settings > Support in Wickr Me. " +
    "Way to go to protect your privacy!",

    "Here is how to find your friends on Wickr Me:\n\n" +
    "Go to New Conversations> tap on the contact you want to message if you see them, or start typing their Wickr ID in " +
    "the contact field. For group conversations: tap and hold on multiple contacts on Android, or select more than 1 " +
    "participants in iOS and desktop.",

    "Here is how to set expiration on your messages:\n\n" +
    "Expiration is the max time your message will live. Burn-On-Read (BOR) is how long your message will live once the " +
    "recipient(s) has seen it. You can change both by tapping on the (i) next to any conversation name, at the top of " +
    "your screen.",

    "Video Verification:\n" +
    "If you'd like to be sure you are talking to the right person, you can send them a verification request. Tap on a user's " +
    "avatar>then on the key icon. You can read more on why key verification is cool for your privacy on our blog: " +
    "https://medium.com/cryptoblog/key-verification-in-secure-messaging-bd93a1bf3d40",

    "How to invite friends to join you on Wickr Me:\n\n" +
    "You can invite your friends by going to your Settings > Contacts > then tap Invite in the top right corner of your app. " +
    "Chose to invite friends by either sending a text or email from your device. They will need to download the app, and " +
    "create a Wickr ID to communicate with you here.\n\n" +
    "We never store your device contacts on our servers. All invitations are generated locally on your device, without " +
    "sharing any information with us.",

    "Sending files on Wickr Me:\n\n" +
    "You can now send photos, videos, and other files via Wickr Me, up to 10 MB. This feature supports collaboration " +
    "and maximum data hygiene for you and the contacts you TRUST. If you do not trust the person you’re talking to, do " +
    "not open files coming from them or send them photos/files you do not want to be saved. Stay safe!",

    "Verification\n\n" +
    "You’ll notice an orange dot around your contacts’ avatars – that means you have not yet verified them.\n\n" +
    "You don’t have to, but in case you want to make sure you are talking to the right person, send them a key video " +
    "verification request to establish trust between your Wickr Me accounts.\n\n" +
    "Check out our blog on this: https://medium.com/cryptoblog/key-verification-in-secure-messaging-bd93a1bf3d40",

    "Passwords\n\n" +
    "Important to know: there is no password reset on Wickr Me – we don't know who you are which prevents us from " +
    "verifying you to reset your password.\n\n" +
    "So please remember your password☺",

    "Client Support\n\n" +
    "You can use Wickr Me on mobile or desktop to stay in touch with your friends across all your devices.\n\n" +
    "Go to www.me-download.wickr.com to download and install on your other devices.",

    "Privacy\n\n" +
    "We built Wickr Me to provide private communications to everyone.\n" +
    "We take your privacy & security very seriously, learn more: www.wickr.com/security.\n\n" +
    "Source code https://github.com/WickrInc/wickr-crypto-c. FAQ www.wickr.com/faq"
  ];

  addon.cmdStartAsyncRecvMessages(listen);

  var wickrUsers = [];

  function listen(message) {
    var parsedData = JSON.parse(message);
    var vGroupID = parsedData.vgroupid;
    var location = find(vGroupID);
    if (location === -1) {
      wickrUsers.push({
        vGroupID: vGroupID,
        index: 0
      });
    }
    var current = getIndex(vGroupID);
    if (current > 9) {
      location = find(vGroupID);
      wickrUsers[location].index = 0;
    }
    current = getIndex(vGroupID);
    if (current <= 9 && current != -1) {
      var csrm = addon.cmdSendRoomMessage(vGroupID, responseMessageList[current], '100', '60');
      location = find(vGroupID);
      wickrUsers[location].index = current + 1;
    }

  }


  function find(vGroupID) {
    for (var i = 0; i < wickrUsers.length; i++) {
      if (wickrUsers[i].vGroupID[0].localeCompare(vGroupID[0]) === 0)
        return i;
    }
    return -1;
  }

  function getIndex(vGroupID) {
    for (var i = 0; i < wickrUsers.length; i++) {
      if (wickrUsers[i].vGroupID[0] === vGroupID[0]) {
        return wickrUsers[i].index;
      }
    }
  }
}).catch(error => {
  console.log('Error: ', error);
});
