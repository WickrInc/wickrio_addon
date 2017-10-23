#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>


#include "cmdhandler.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"

/**
 * @brief CmdHandler::CmdHandler
 *
 * @param parent
 */
CmdHandler::CmdHandler(QObject *parent) :
    HttpRequestHandler(parent)
{
}

/**
 * @brief CmdHandler::~CmdHandler
 * Destructor for the Command Handler class. Make sure that all memory is deallocated!
 */
CmdHandler::~CmdHandler() {
}

/**
 * @brief CmdHandler::service
 * This function will parse the incoming HTTP Request.
 * @param request
 * @param response
 */
void CmdHandler::service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response) {

    QByteArray path=request.getPath();
    QByteArray method=request.getMethod();

    qDebug() << "Got Message:" << request.getBody();

    response.setStatus(200, "Success");
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");
    response.write("",true);
}
