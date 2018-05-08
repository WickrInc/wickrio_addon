#include "requestHandlerJS.h"

using namespace v8;

RequestHandlerJS::RequestHandlerJS(OperationData *operation) :
    m_apiInterface(operation)
{
}

void
RequestHandlerJS::sendMessage(const FunctionCallbackInfo<Value>& args)
{
    Isolate * isolate = args.GetIsolate();

    v8::String::Utf8Value s(args[0]);

    std::string str(*s, s.length());

    QByteArray jsonString = QByteArray::fromStdString(str);
    QString responseString;

    m_success = m_apiInterface.sendMessage(jsonString, responseString);
    m_response = responseString.toStdString().c_str();

    Local<String> retval = String::NewFromUtf8(isolate, responseString.toStdString().c_str());
    args.GetReturnValue().Set(retval);
}
