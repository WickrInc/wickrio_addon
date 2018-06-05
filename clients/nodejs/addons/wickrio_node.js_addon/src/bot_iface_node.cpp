#include <nan.h>
#include <v8.h>
#include "bot_iface.h"

using namespace v8;
using namespace std;
using namespace Nan;


std::string client;
BotIface *botIface = nullptr;

void clientInit(const v8::FunctionCallbackInfo<v8::Value> & args) {
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string str = std::string(*param1);
        client = str;
        botIface = new BotIface(client);
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
}

void cmdGetStatistics(const v8::FunctionCallbackInfo<v8::Value> & args){
        string command, response;

        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }

        botIface->cmdStringGetStatistics(command);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdClearStatistics(const v8::FunctionCallbackInfo<v8::Value> & args){
        string command, response;

        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }

        botIface->cmdStringClearStatistics(command);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdGetRooms(const v8::FunctionCallbackInfo<v8::Value> & args){
        string command, response;

        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }

        botIface->cmdStringGetRooms(command);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdAddRoom(const v8::FunctionCallbackInfo<v8::Value> & args) {
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[1]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param2(item->ToString());
                std::string str = std::string(*param2);
                members.push_back(str);
        }
        vector <string> moderators;
        Local<Array> arr1 = Local<Array>::Cast(args[2]);
        for(int j=0; j<arr1->Length(); j++) {
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

        botIface->cmdStringAddRoom(command, members, moderators, title, description, ttl, bor);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}


void cmdModifyRoom(const v8::FunctionCallbackInfo<v8::Value> & args) {
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[1]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param2(item->ToString());
                std::string str = std::string(*param2);
                members.push_back(str);
        }
        vector <string> moderators;
        Local<Array> arr1 = Local<Array>::Cast(args[2]);
        for(int j=0; j<arr1->Length(); j++) {
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

        botIface->cmdStringModifyRoom(command, vGroupID, members, moderators, title, description, ttl, bor);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}


void cmdGetRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringGetRoom(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdLeaveRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringLeaveRoom(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdDeleteRoom(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringDeleteRoom(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdAddGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args) {
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        vector <string> members;
        Local<Array> arr = Local<Array>::Cast(args[0]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param1(item->ToString());
                std::string str = std::string(*param1);
                members.push_back(str);
        }
        v8::String::Utf8Value param2(args[1]->ToString());
        std::string ttl = std::string(*param2);
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string bor = std::string(*param3);

        botIface->cmdStringAddGroupConvo(command, members, ttl, bor);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}


void cmdDeleteGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringDeleteGroupConvo(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdGetGroupConvo(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        botIface->cmdStringGetGroupConvo(command, vGroupID);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}


void cmdGetGroupConvos(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        botIface->cmdStringGetGroupConvos(command);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void cmdGetReceivedMessage(const v8::FunctionCallbackInfo<v8::Value> & args){
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        botIface->cmdStringGetReceivedMessage(command);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}


void cmdSendMessage(const v8::FunctionCallbackInfo<v8::Value> & args) {
        if ( botIface->init() != BotIface::SUCCESS) {
                std::cout << "Could not initialize Bot Interface!";
                std::cout << botIface->getLastErrorString();
                return;
        }
        string command, response;
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string vGroupID = std::string(*param1);
        vector <string> users;
        Local<Array> arr = Local<Array>::Cast(args[1]);
        for(int i=0; i<arr->Length(); i++) {
                Local<Value> item = arr->Get(i);
                v8::String::Utf8Value param2(item->ToString());
                std::string str = std::string(*param2);
                users.push_back(str);
        }
        v8::String::Utf8Value param3(args[2]->ToString());
        std::string message = std::string(*param3);
        v8::String::Utf8Value param4(args[3]->ToString());
        std::string ttl = std::string(*param4);
        v8::String::Utf8Value param5(args[4]->ToString());
        std::string bor = std::string(*param5);

        botIface->cmdStringSendMessage(command, vGroupID, users, message, ttl, bor);

        if (botIface->send(command, response) != BotIface::SUCCESS) {
                response = botIface->getLastErrorString();
                std::cout << "Send failed: " << response;
        }
        else {
                if (response.length() > 0) {
                        cout << response << endl;
                }
        }
        return;
}

void init(Handle <Object> exports, Handle<Object> module) {

        //2nd param: what we call from Javascript
        //3rd param: the name of the actual function
        NODE_SET_METHOD(exports, "clientInit", clientInit);
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
        NODE_SET_METHOD(exports, "cmdSendMessage", cmdSendMessage);
}

// associates the module name with initialization logic
NODE_MODULE(wickrio_addon, init)
