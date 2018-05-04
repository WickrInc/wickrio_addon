// contents of addon_source.cc

// This is a basic addon - a binding.gyp file 
// would need to include this file as it's source.

#include <nan.h>
using namespace std;
using namespace Nan;
using namespace v8;

// Accepts 1 number from JavaScript, adds 42 and returns the result.
NAN_METHOD(SendMessage) {
    if ( info.Length() < 1 ) {
        return;
    }
    if ( !info[0]->IsString()) {
        return;
    }

    // wrap the string with v8's string type
    v8::String::Utf8Value val(info[0]->ToString());

    std::string retString = Client::Call(str);

    info.GetReturnValue().Set(Nan::New<String>(retString.c_str()).ToLocalChecked()); 
}

// Called by the NODE_MODULE macro below, 
// exposes a pass_number method to JavaScript, which maps to SendMessage above.
//
NAN_MODULE_INIT(Init) {
    Nan::Set(target, New<String>("send_message").ToLocalChecked(),
             GetFunction(New<FunctionTemplate>(SendMessage)).ToLocalChecked());
}

// macro to load the module when require'd
NODE_MODULE(wickrio_addon, Init)
  
