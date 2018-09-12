#include <iostream>
#include <nan.h>
#include <v8.h>
#include <node.h>
#include <uv.h>
#include <thread>
#include <node_api.h>
#include "bot_iface.h"

using namespace v8;
using namespace std;
using namespace Nan;

typedef struct napi_env__ *napi_env;
// static Nan::CopyablePersistentTraits<v8::Function>::CopyablePersistent _cb;

BotIface *botIface = nullptr;
Isolate* isolate = nullptr;
// v8::Persistent<Object> persist;

 // v8::ReturnValue<v8::Value> returnValue = v8::ReturnValue<v8::Value>();
// const v8::FunctionCallbackInfo<v8::Value> globalArgs;
string jsCallback;
v8::Local<v8::Value> globalArgs;


void clientInit(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        string message;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string client = std::string(*param1);
        botIface = new BotIface(client);
        if (botIface->init() != BotIface::SUCCESS) {
          message = botIface->getLastErrorString() + "\nCould not initialize Bot Interface!";
        }
        else{
          message = "Bot Interface initialized successfully!";
        }
        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
        args.GetReturnValue().Set(error);
        return;
}

void closeClient(const v8::FunctionCallbackInfo<v8::Value> & args){
  Isolate* isolate = args.GetIsolate();
  delete botIface;
  botIface = NULL;
  string message = "Bot Interface Client closed successfully!";
  auto error = v8::String::NewFromUtf8(isolate, message.c_str());
  args.GetReturnValue().Set(error);
  return;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////ASYNC MESSAGE CALLBACK HANDLING///////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// std::function<void()> cbfunc;
//
//
// This function gets passed to cmdStringStartAsyncRecvMessages in bot_iface.cpp as a regular C++ function
void callback(string msg){
  cout << "callback()\n";
  // Get the js callback function on the global object
  // v8::String::Utf8Value param1(globalArgs->ToString());
  // std::string args = std::string(*param1);
  cout <<"callback isolate:" <<isolate << endl;

  // cout << "globalArgs:" << args;
  //
  // v8::Locker locker(isolate);
  //   isolate->Enter();
  //   v8::HandleScope handleScope(isolate);
    // Local<Value> key = String::NewFromUtf8(isolate, msg.c_str());
    // Local<Function> target = Local<Function>::New(isolate, persist);
    // target->Set(key);
  const int argc = 1;
  v8::Handle<v8::Value> argv[argc] = {  v8::String::NewFromUtf8(isolate, msg.c_str()) };
  cout << "1\n";
  cout << isolate->GetCurrentContext();
  v8::Handle<v8::Value> value = isolate->GetCurrentContext()->Global()->Get(v8::String::NewFromUtf8(isolate,"printer"));
  cout << "2\n";
  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value);
  cout << "3\n";

  // Local<Function> callback = Local<Function>::Cast(globalArgs);
  func->Call(isolate->GetCurrentContext(),value, argc, argv);
  cout << "4\n";
  // auto message = v8::String::NewFromUtf8(isolate, msg.c_str());
  // globalArgs.GetReturnValue().Set(message);

    // Create a new context.
//     v8::Local<v8::Context> context = v8::Context::New(isolate);
//
// napi_value global, js_func, arg;
// napi_env env = new napi_env(isolate);
//  // env = GetEnv(context);
// napi_status status = napi_get_global(env, &global);
// cout << endl <<"env:" << env << endl;
// const napi_extended_error_info* result1;
// status = napi_get_last_error_info(env, &result1);
//
// cout << "result1:" << result1 << endl;
// cout << "status1:" << status << endl;
// // if (status != napi_ok) return;
// cout << "jsCallback:" << jsCallback.c_str() << endl;
// status = napi_get_named_property(env, global, jsCallback.c_str(), &js_func);
// cout << "global:" << global << endl;
// cout << "&js_func:" << &js_func << endl;
// // cout << "status:" << status << endl;
// // if (status != napi_ok) return;
//
// // const arg = msg
// status = napi_create_string_utf8(env, msg.c_str(), NAPI_AUTO_LENGTH, &arg);
// cout << "NAPI_AUTO_LENGTH:" << NAPI_AUTO_LENGTH << endl;
// cout << "msg:" << msg << endl;
// cout << "arg:" << arg << endl;
// // if (status != napi_ok) return;
//
// napi_value* argv = &arg;
// size_t argc = 1;
//
// // jsCallback(arg);
// napi_value return_val;
// status = napi_call_function(env, global, js_func, argc, argv, &return_val);
// cout << "status after call:" << status << endl;
// cout << "return_val:" << return_val << endl;
// // if (status != napi_ok) return;
//
// // Convert the result back to a native type
// size_t result;
// char str[1024];
// status = napi_get_value_string_utf8(env, return_val, (char *) &str, 1024, &result);
// cout << "status2:" << status << endl;
// cout << result << endl;
// if (status != napi_ok) return;
}
// Addon function which gets called from Javascript and gets passed a callback
//
// See if you can save const v8::FunctionCallbackInfo<v8::Value> & args as a global variable and use it later in callback()

//
void cmdStartAsyncRecvMessages(const v8::FunctionCallbackInfo<v8::Value> & args){
  // _cb = Nan::Persistent<v8::Function>(args[0].As<v8::Function>());

  isolate = args.GetIsolate();
  cout <<"cmd isolate:" <<isolate << endl;
  // persist.Reset(isolate, args[0]->ToObject());

  // globalArgs = args[0];
  globalArgs = args[0];
  // globalArgs = args;
  v8::String::Utf8Value param1(args[0]->ToString());
  jsCallback = std::string(*param1);
  // cout << jsCallback << endl;
string command, response;
botIface->cmdStringStartAsyncRecvMessages(command, callback); //callback needs to be a C++ function and not a V8 function



  if (botIface == nullptr) {
          string message = "Bot Interface has not been initialized!";
          auto error = v8::String::NewFromUtf8(isolate, message.c_str());
          args.GetReturnValue().Set(error);
          return;
  }

  if (botIface->send(command, response) != BotIface::SUCCESS) {
          response = botIface->getLastErrorString();
          string message = "Send failed: " + response;
          auto error = v8::String::NewFromUtf8(isolate, message.c_str());
          args.GetReturnValue().Set(error);
          return;
  }
  else {
          if (response.length() > 0) {
                  auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                  args.GetReturnValue().Set(message);
          }
          return;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


void cmdGetStatistics(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        botIface->cmdStringGetStatistics(command);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Send failed: " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdClearStatistics(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        botIface->cmdStringClearStatistics(command);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Send failed: " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdGetRooms(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        botIface->cmdStringGetRooms(command);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Send failed: " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdAddRoom(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 4) {
                string message = "AddRoom: requires at least 4 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsArray() || !args[1]->IsArray() || !args[2]->IsString() || !args[3]->IsString()) {
                string message;
                if(!args[0]->IsArray())
                        message = "AddRoom: list of Members must be an array!";
                if(!args[1]->IsArray())
                        message = "AddRoom: list of Moderators must be an array!";
                if(!args[2]->IsString())
                        message = "AddRoom: Title must be a string!";
                if(!args[3]->IsString())
                        message = "AddRoom: Description must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if(args.Length() == 5) {
                string message;
                if(!args[4]->IsString()) {
                        message= "AddRoom: ttl must be a string!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        if(args.Length() == 6) {
                string message;
                if(!args[4]->IsString())
                        message = "AddRoom: ttl must be a string!";
                else if(!args[5]->IsString())
                        message = "AddRoom: bor must be a string!";
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response, ttl, bor;
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[0]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                if(!item->IsString()) {
                        string message = "AddRoom: member IDs must be strings!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param1(item->ToString());
                std::string str = std::string(*param1);
                members.push_back(str);
        }
        vector <string> moderators;
        Local<Array> arr1 = Local<Array>::Cast(args[1]);
        for(int j=0; j<arr1->Length(); j++) {
                Local<Value> item = arr1->Get(j);
                if(!item->IsString()) {
                        string message = "AddRoom: moderator IDs must be strings!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param2(item->ToString());
                std::string str = std::string(*param2);
                moderators.push_back(str);
        }
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string title = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string description = std::string(*param4);
        if(args.Length() >= 5) {
                v8::String::Utf8Value param5(args[4]->ToString());
                ttl = std::string(*param5);
        }
        if(args.Length() == 6) {
                v8::String::Utf8Value param6(args[5]->ToString());
                bor = std::string(*param6);
        }
        botIface->cmdStringAddRoom(command, members, moderators, title, description, ttl, bor);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Add Room command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdModifyRoom(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 2) {
                string message = "ModifyRoom: requires at least 2 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        for(int i = 0; i < args.Length() - 1; i++) {
                string message;
                if(i == 0) {
                        if(!args[i]->IsString()) {
                                message = "ModifyRoom: vGroupID must be a string!";
                        }
                }
                else if(i == 1) {
                        if(!args[i]->IsArray()) {
                                message = "ModifyRoom: list of Members must be an array!";
                        }
                }
                else if(i == 2) {
                        if(!args[i]->IsArray()) {
                                message = "ModifyRoom: list of Moderators must be an array!";
                        }
                }
                else if(i == 3) {
                        if(!args[i]->IsString()) {
                                message = "ModifyRoom: Title must be a string!";
                        }
                }
                else if(i == 4) {
                        if(!args[i]->IsString()) {
                                message = "ModifyRoom: Description must be a string!";
                        }
                }
                else if(i == 5) {
                        if(!args[i]->IsString()) {
                                message = "ModifyRoom: ttl must be a string!";
                        }
                }
                else if(i == 6) {
                        if(!args[i]->IsString()) {
                                message = "ModifyRoom: bor must be a string!";
                        }
                }
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response, ttl, bor;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[1]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                if(!item->IsString()) {
                        string message = "ModifyRoom: member IDs must be strings!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param2(item->ToString());
                std::string str = std::string(*param2);
                members.push_back(str);
        }
        vector <string> moderators;
        Local<Array> arr1 = Local<Array>::Cast(args[2]);
        for(int j=0; j<arr1->Length(); j++) {
                Local<Value> item = arr1->Get(j);
                if(!item->IsString()) {
                        string message = "ModifyRoom: moderator IDs must be strings!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param3(item->ToString());
                std::string str = std::string(*param3);
                moderators.push_back(str);
        }
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string title = std::string(*param4);
        v8::String::Utf8Value param5(args[4]->ToString());
        std::string description = std::string(*param5);
        v8::String::Utf8Value param6(args[5]->ToString());
        ttl = std::string(*param6);
        v8::String::Utf8Value param7(args[6]->ToString());
        bor = std::string(*param7);
        botIface->cmdStringModifyRoom(command, vGroupID, members, moderators, title, description, ttl, bor);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Modify Room command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdGetRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "GetRoom: requires 1 vGroupID argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsString()) {
                string message = "GetRoom: VGroupID must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);

        botIface->cmdStringGetRoom(command, vGroupID);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Get Room command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdLeaveRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "LeaveRoom: requires 1 vGroupID argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsString()) {
                string message = "LeaveRoom: VGroupID must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringLeaveRoom(command, vGroupID);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Leave Room command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdDeleteRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "DeleteRoom: requires 1 vGroupID argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsString()) {
                string message = "DeleteRoom: VGroupID must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringDeleteRoom(command, vGroupID);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Delete Room command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdAddGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "AddGroupConvo: requires at least 1 argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsArray()) {
                string message = "AddGroupConvo: list of Members must be an array!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response, ttl, bor;
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[0]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                if(!item->IsString()) {
                        string message = "AddgroupConvo: member IDs must be strings!";
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param1(item->ToString());
                std::string str = std::string(*param1);
                members.push_back(str);
        }
        if(args.Length() == 2) {
                string message;
                if(!args[1]->IsString())
                        message= "AddGroupConvo: ttl must be a string!";
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param2(args[1]->ToString());
                ttl = std::string(*param2);
        }
        if(args.Length() == 3) {
                string message;
                if(!args[1]->IsString())
                        message = "AddGroupConvo: ttl must be a string!";
                else if(!args[2]->IsString())
                        message = "AddGroupConvo: bor must be a string!";
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
                v8::String::Utf8Value param2(args[1]->ToString());
                ttl = std::string(*param2);
                v8::String::Utf8Value param3(args[2]->ToString());
                bor = std::string(*param3);
        }
        botIface->cmdStringAddGroupConvo(command, members, ttl, bor);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Add Group Conversation command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdDeleteGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "DeleteGroupConvo: requires 1 vGroupID argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsString()) {
                string message = "DeleteGroupConvo: VGroupID must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);

        botIface->cmdStringDeleteGroupConvo(command, vGroupID);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Delete Group Conversaion command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdGetGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 1) {
                string message = "GetGroupConvo: requires 1 vGroupID argument!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (!args[0]->IsString()) {
                string message = "GetGroupConvo: VGroupID must be a string!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringGetGroupConvo(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Get Group Conversaion command! " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                  auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                  args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdGetGroupConvos(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }

        string command, response;
        botIface->cmdStringGetGroupConvos(command);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Send failed: " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

void cmdGetReceivedMessage(const v8::FunctionCallbackInfo<v8::Value> & args){
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        string command, response;
        botIface->cmdStringGetReceivedMessage(command);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Send failed: " + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdSend1to1Message(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 2) {
                string message = "Send1to1Message: requires at least 2 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }

        for(int i = 0; i < args.Length() - 1; i++) {
                string message;
                if(i == 0) {
                        if(!args[i]->IsArray()) {
                                message = "Send1to1Message: list of users must be an array!";
                        }
                }
                else if(i == 1) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Message: message must be a string!";
                        }
                }
                else if(i == 2) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Message: ttl must be a string!";
                        }
                }
                else if(i == 3) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Message: bor must be a string!";
                        }
                }
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response;
        std::string placeHolder;
        vector <string> users;
        Local<Array> arr = Local<Array>::Cast(args[0]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param1(item->ToString());
                std::string str = std::string(*param1);
                users.push_back(str);
        }
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string message = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string ttl = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string bor = std::string(*param4);
        botIface->cmdStringSendMessage(command, placeHolder, users, message, ttl, bor);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Send 1-to-1 Message command!" + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}




void cmdSend1to1Attachment(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 2) {
                string message = "Send1to1Attachment: requires at least 2 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        for(int i = 0; i < args.Length() - 1; i++) {
                string message;
                if(i == 0) {
                        if(!args[i]->IsArray()) {
                                message = "Send1to1Attachment: list of users must be an array!";
                        }
                }
                else if(i == 1) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Attachment: attachment must be a string!";
                        }
                }
                else if(i == 2) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Attachment: displayname must be a string!";
                        }
                }
                else if(i == 3) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Attachment: ttl must be a string!";
                        }
                }
                else if(i == 4) {
                        if(!args[i]->IsString()) {
                                message = "Send1to1Attachment: bor must be a string!";
                        }
                }
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response;
        std::string placeHolder;
        vector <string> users;
        Local<Array> arr = Local<Array>::Cast(args[0]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param1(item->ToString());
                std::string str = std::string(*param1);
                users.push_back(str);
        }
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string attachment = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string displayname = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string ttl = std::string(*param4);
        v8::String::Utf8Value param5(args[4]->ToString());
        std::string bor = std::string(*param5);
        if(displayname.length() > 0){
          botIface->cmdStringSendAttachment(command, placeHolder, users, attachment, displayname, ttl, bor);
        }
        else{
          botIface->cmdStringSendAttachment(command, placeHolder, users, attachment, placeHolder, ttl, bor);
        }
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Send 1-to-1 Attachment command!" + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdSendRoomMessage(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 2) {
                string message = "SendRoomMessage: requires at least 2 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }

        for(int i = 0; i < args.Length() - 1; i++) {
                string message;
                if(i == 0) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomMessage: vGroupID must be a string!";
                        }
                }
                else if(i == 1) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomMessage: message must be a string!";
                        }
                }
                else if(i == 2) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomMessage: ttl must be a string!";
                        }
                }
                else if(i == 3) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomMessage: bor must be a string!";
                        }
                }
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        vector <string> placeHolder;
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string message = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string ttl = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string bor = std::string(*param4);
        botIface->cmdStringSendMessage(command, vGroupID, placeHolder, message, ttl, bor);
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Send Room Message command!" + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}


void cmdSendRoomAttachment(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        if (botIface == nullptr) {
                string message = "Bot Interface has not been initialized!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        if (args.Length() < 2) {
                string message = "SendRoomAttachment: requires at least 2 arguments!";
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }

        for(int i = 0; i < args.Length() - 1; i++) {
                string message;
                if(i == 0) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomAttachment: vGroupID must be a string!";
                        }
                }
                else if(i == 1) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomAttachment: attachment must be a JSON object!";
                        }
                }
                else if(i == 2) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomAttachment: ttl must be a string!";
                        }
                }
                else if(i == 3) {
                        if(!args[i]->IsString()) {
                                message = "SendRoomAttachment: bor must be a string!";
                        }
                }
                if(!message.empty()) {
                        auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                        args.GetReturnValue().Set(error);
                        return;
                }
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        vector <string> placeHolder;
        std::string stringHolder;
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string attachment = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string displayname = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string ttl = std::string(*param4);
        v8::String::Utf8Value param5(args[4]->ToString());
        std::string bor = std::string(*param5);
        if(displayname.length() > 0){
          botIface->cmdStringSendAttachment(command, vGroupID, placeHolder, attachment, displayname, ttl, bor);
        }
        else{
          botIface->cmdStringSendAttachment(command, vGroupID, placeHolder, attachment, displayname, ttl, bor);
        }
        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                string message = "Failed to create Send Room Attachment command!" + response;
                auto error = v8::String::NewFromUtf8(isolate, message.c_str());
                args.GetReturnValue().Set(error);
                return;
        }
        else {
                if (response.length() > 0) {
                        auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                        args.GetReturnValue().Set(message);
                }
                return;
        }
}

// void sendBackToJs(const v8::FunctionCallbackInfo<v8::Value> & args);
void init(Handle <Object> exports, Handle<Object> module) {
        //2nd param: what we call from Javascript
        //3rd param: the name of the actual function
        NODE_SET_METHOD(exports, "clientInit", clientInit);
        NODE_SET_METHOD(exports, "closeClient", closeClient);
        // NODE_SET_METHOD(module, "exports", callback);
        // NODE_SET_METHOD(module, "sendBackToJs", sendBackToJs);
        NODE_SET_METHOD(exports, "cmdStartAsyncRecvMessages", cmdStartAsyncRecvMessages);
        NODE_SET_METHOD(exports, "cmdGetStatistics", cmdGetStatistics);
        NODE_SET_METHOD(exports, "cmdClearStatistics", cmdClearStatistics);
        NODE_SET_METHOD(exports, "cmdGetRooms", cmdGetRooms);
        NODE_SET_METHOD(exports, "cmdAddRoom", cmdAddRoom);
        NODE_SET_METHOD(exports, "cmdModifyRoom", cmdModifyRoom);
        NODE_SET_METHOD(exports, "cmdGetRoom", cmdGetRoom);
        NODE_SET_METHOD(exports, "cmdLeaveRoom", cmdLeaveRoom);
        NODE_SET_METHOD(exports, "cmdDeleteRoom", cmdDeleteRoom);
        NODE_SET_METHOD(exports, "cmdAddGroupConvo", cmdAddGroupConvo);
        NODE_SET_METHOD(exports, "cmdDeleteGroupConvo", cmdDeleteGroupConvo);
        NODE_SET_METHOD(exports, "cmdGetGroupConvo", cmdGetGroupConvo);
        NODE_SET_METHOD(exports, "cmdGetGroupConvos", cmdGetGroupConvos);
        NODE_SET_METHOD(exports, "cmdGetReceivedMessage", cmdGetReceivedMessage);
        NODE_SET_METHOD(exports, "cmdSend1to1Message", cmdSend1to1Message);
        NODE_SET_METHOD(exports, "cmdSend1to1Attachment", cmdSend1to1Attachment);
        NODE_SET_METHOD(exports, "cmdSendRoomMessage", cmdSendRoomMessage);
        NODE_SET_METHOD(exports, "cmdSendRoomAttachment", cmdSendRoomAttachment);
}

// associates the module name with initialization logic
NODE_MODULE(wickrio_addon, init)
