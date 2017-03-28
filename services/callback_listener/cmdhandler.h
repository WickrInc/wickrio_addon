#ifndef CMDHANDLER_H
#define CMDHANDLER_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include <httpserver/httprequesthandler.h>
#include "wickrbotipc.h"
#include "wickrbotdatabase.h"
#include "operationdata.h"

#include "httpserver/httplistener.h"

class CmdHandler : public stefanfrings::HttpRequestHandler
{
    Q_OBJECT
public:
    CmdHandler(QObject *parent);
    ~CmdHandler();
        /**
      Process an incoming HTTP request.
      @param request The received HTTP request
      @param response Must be used to return the response
    */
    void service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
};

#endif // CMDHANDLER_H
