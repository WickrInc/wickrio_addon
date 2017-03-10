#ifndef WICKRIOHTTP_H
#define WICKRIOHTTP_H

#include <QObject>
#include <httpserver/httprequesthandler.h>
#include "wickriodatabase.h"

class WickrIOHttpRequestHdlr : public HttpRequestHandler {
    Q_OBJECT
    Q_DISABLE_COPY(WickrIOHttpRequestHdlr)

public:
    WickrIOHttpRequestHdlr(QObject* parent=NULL);

    void setupResponse(HttpRequest& request, HttpResponse& response);
    void sendSuccess(QByteArray data, HttpResponse& response, bool lastPart=true);
    void sendSuccess(HttpResponse& response, bool lastPart=true);
    void sendAccepted(HttpResponse& response, bool lastPart=true);
    void sendFailure(int status, QByteArray msg, HttpResponse& response, bool lastPart=true);
    void optionsResponse(HttpRequest& request, HttpResponse& response);

    bool validateAuthentication(HttpRequest& request, HttpResponse& response, WickrIOConsoleUser *cUser);

private:
    static QByteArray hmac_sha256(const QByteArray &key, const QByteArray &secret);

protected:
    // Functions shared by clients
    void setMsgRecvCallback(const QString& apiKey, HttpRequest& request, HttpResponse& response);
    void getMsgRecvCallback(const QString& apiKey, HttpResponse& response);
    void deleteMsgRecvCallback(const QString& apiKey, HttpResponse& response);
    void setMsgRecvEmail(const QString& apiKey, HttpRequest& request, HttpResponse& response);
    void getMsgRecvEmail(const QString& apiKey, HttpResponse& response);
    void deleteMsgRecvEmail(const QString& apiKey, HttpResponse& response);

    WickrIOClientDatabase *m_ioDB;

};

#endif // WICKRIOHTTP_H
