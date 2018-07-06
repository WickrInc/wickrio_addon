#include <nan.h>
#include <v8.h>
#include "bot_iface.h"

using namespace v8;
using namespace std;
using namespace Nan;

BotIface *botIface = nullptr;

void clientInit(const v8::FunctionCallbackInfo<v8::Value> & args) {
        Isolate* isolate = args.GetIsolate();
        string message;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string client = std::string(*param1);
        botIface = new BotIface(client);
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string amqp_user = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string amqp_password = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string amqp_address = std::string(*param4);
        v8::String::Utf8Value param5(args[4]->ToString());
        std::string amqp_port = std::string(*param5);
        std::string amqp = amqp_user + ":" + amqp_password + "@" + amqp_address + ":" + amqp_port;
        cout << endl << "amqp: " + amqp << endl;
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
//v8::Handle<Boolean>
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
        cout << endl << vGroupID << endl << members[0] << endl <<title << endl << description << endl << ttl << endl << bor << endl;
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
                // else{
                //   response = "cmdSendRoomMessage Success!";
                //   auto message = v8::String::NewFromUtf8(isolate, response.c_str());
                //   args.GetReturnValue().Set(message);
                // }
                return;
        }
}

void init(Handle <Object> exports, Handle<Object> module) {
        //2nd param: what we call from Javascript
        //3rd param: the name of the actual function
        NODE_SET_METHOD(exports, "clientInit", clientInit);
        NODE_SET_METHOD(exports, "closeClient", closeClient);
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
        NODE_SET_METHOD(exports, "cmdSendRoomMessage", cmdSendRoomMessage);
}

// associates the module name with initialization logic
NODE_MODULE(wickrio_addon, init)
