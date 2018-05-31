#include <nan.h>
#include <v8.h>
#include "bot_iface.h"

using namespace v8;
using namespace std;
using namespace Nan;


void display(const v8::FunctionCallbackInfo<v8::Value> & args) {
  Isolate* isolate = args.GetIsolate();

  std::string client = std::string("name");
  BotIface botIface(client);
  //v8::Handle<v8::Int> a = v8::Handle<v8::int>::Cast(args[0]);
  //v8::Handle<v8::int> b = v8::Handle<v8::int>::Cast(args[1]);
  v8::String::Utf8Value param1(args[0]->ToString());
  std::string command = std::string(*param1);
  botIface.display(command);

  //Local<Number> retval = v8::Number::New(isolate, result);
  //args.GetReturnValue().Set(retval);
}



BotIface::BotIfaceStatus send(const v8::FunctionCallbackInfo<v8::Value> & args) {
  Isolate* isolate = args.GetIsolate();
  std::string client = std::string("name");
  BotIface botIface(client);

  v8::String::Utf8Value param1(args[0]->ToString());
  std::string command = std::string(*param1);
  v8::String::Utf8Value param2(args[1]->ToString());
  std::string response = std::string(*param2);

  string response;
  if (botIface.send(command, response) != BotIface::SUCCESS) {
      std::cout << "Send failed: " << botIface.getLastErrorString();
      continue;
  }

  Local<Boolean> retval = v8::Boolean::New(isolate, result);
  args.GetReturnValue().Set(retval);
}



BotIface::BotIfaceStatus cmdAddRoom(const v8::FunctionCallbackInfo<v8::Value> & args) {
  Isolate* isolate = args.GetIsolate();
  std::string client = std::string("name");
  BotIface botIface(client);

 std::string command;

  vector <string> members;
  Local<Array> arr = Local<Array>::Cast(args[1]);
  for(int i=0; i<arr->Length();i++){
    Local<Value> item = arr->Get(i);
    v8::String::Utf8Value param2(item->ToString());
    std::string str = std::string(*param2);
    members.push_back(str);
  }
  vector <string> moderators;
  Local<Array> arr1 = Local<Array>::Cast(args[2]);
  for(int j=0; j<arr1->Length();j++){
    Local<Value> item = arr1->Get(j);
    v8::String::Utf8Value param3(item->ToString());
    std::string str = std::string(*param3);
    moderators.push_back(str);
  }
  v8::String::Utf8Value param4(args[3]->ToString());
  std::string title = std::string(*param4);
  v8::String::Utf8Value param5(args[4]->ToString());
  std::string description = std::string(*param5);
  v8::String::Utf8Value param6(args[5]->ToString());
  std::string ttl = std::string(*param6);
  v8::String::Utf8Value param7(args[6]->ToString());
  std::string bor = std::string(*param7);

  BotIface::BotIfaceStatus result = botIface.cmdStringAddRoom(command, members, moderators, title, description, ttl, bor);

  BotIface::BotIfaceStatus retval = v8::BotIface::BotIfaceStatus::New(isolate, result);
  args.GetReturnValue().Set(retval);
}



void init(Handle <Object> exports, Handle<Object> module) {
  std::string client = std::string("name");
  BotIface botIface(client);
  NODE_SET_METHOD(exports, "display", display);
  NODE_SET_METHOD(exports, "send", send);
  NODE_SET_METHOD(exports, "cmdAddRoom", cmdAddRoom);
  // NODE_SET_METHOD(exports, "cmdStringModifyRoom", cmdStringModifyRoom);
  // NODE_SET_METHOD(exports, "cmdStringDeleteRoom", cmd_string_delete_room);
  // NODE_SET_METHOD(exports, "cmdStringLeaveRoom", cmd_string_leave_room);
  // NODE_SET_METHOD(exports, "cmdStringGetRooms", cmd_string_get_rooms);
}


// associates the module name with initialization logic
NODE_MODULE(wickrio_addon, init)
