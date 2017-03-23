#ifndef WICKRIOHTTP_H
#define WICKRIOHTTP_H

#include <QObject>
#include <httpserver/httprequesthandler.h>
#include "wickriodatabase.h"

class WickrIOHttpRequestHdlr : public stefanfrings::HttpRequestHandler {
    Q_OBJECT
    Q_DISABLE_COPY(WickrIOHttpRequestHdlr)

public:
    WickrIOHttpRequestHdlr(QObject* parent=NULL);

    void setupResponse(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void sendSuccess(QByteArray data, stefanfrings::HttpResponse& response, bool lastPart=true);
    void sendSuccess(stefanfrings::HttpResponse& response, bool lastPart=true);
    void sendAccepted(stefanfrings::HttpResponse& response, bool lastPart=true);
    void sendFailure(int status, QByteArray msg, stefanfrings::HttpResponse& response, bool lastPart=true);
    void optionsResponse(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);

    bool validateAuthentication(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response, WickrIOConsoleUser *cUser);

private:
    static QByteArray hmac_sha256(const QByteArray &key, const QByteArray &secret);

protected:
    // Functions shared by clients
    void setMsgRecvCallback(const QString& apiKey, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void getMsgRecvCallback(const QString& apiKey, stefanfrings::HttpResponse& response);
    void deleteMsgRecvCallback(const QString& apiKey, stefanfrings::HttpResponse& response);
    void setMsgRecvEmail(const QString& apiKey, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void getMsgRecvEmail(const QString& apiKey, stefanfrings::HttpResponse& response);
    void deleteMsgRecvEmail(const QString& apiKey, stefanfrings::HttpResponse& response);

    WickrIOClientDatabase *m_ioDB;

};

#endif // WICKRIOHTTP_H
