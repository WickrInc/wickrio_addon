#ifndef REQUESTHANDLERJS_H
#define REQUESTHANDLERJS_H

#include "wickrIOAPIInterface.h"
#include "operationdata.h"

#include "v8.h"

using namespace v8;

class RequestHandlerJS
{
public:
    RequestHandlerJS(OperationData *operation);

    void sendMessage(const FunctionCallbackInfo<Value>& args);

    bool        m_success;
    std::string m_response;

private:
    OperationData       *m_operation;
    WickrIOAPIInterface m_apiInterface;

};

#endif // REQUESTHANDLERJS_H
