#ifndef CMDHANDLER_H
#define CMDHANDLER_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "wickriohttp.h"
#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "operationdata.h"
#include "wickrioconsoleuser.h"

#include "httpserver/httplistener.h"

typedef enum { CHECK_DIR, CHECK_FILE, CHECK_NONE, CHECK_INT } CheckType;

class CmdHandler : public WickrIOHttpRequestHdlr
{
    Q_OBJECT
    Q_DISABLE_COPY(CmdHandler)
public:
    CmdHandler(QSettings *settings, OperationData *operation, QObject *parent);
    ~CmdHandler();
        /**
      Process an incoming HTTP request.
      @param request The received HTTP request
      @param response Must be used to return the response
    */
    void service(HttpRequest& request, HttpResponse& response);

private:
    // These function handle the specific API actions
    void addClient(WickrIOConsoleUser *pCUser, HttpRequest& request, HttpResponse& response);
    void deleteClient(WickrIOConsoleUser *pCUser, const QString& clientID, HttpResponse& response);
    void getClients(WickrIOConsoleUser *pCUser, HttpResponse& response);
    void getClient(WickrIOConsoleUser *pCUser, const QString& clientID, HttpResponse& response);
    void updateClient(WickrIOConsoleUser *pCUser, const QString& clientID, HttpRequest& request, HttpResponse& response);

    void addUser(HttpRequest& request, HttpResponse& response);
    void deleteUser(const QString& UserID, HttpResponse& response);
    void getUsers(HttpResponse& response);
    void getUser(const QString& UserID, HttpResponse& response);
    void updateUser(const QString& UserID, HttpRequest& request, HttpResponse& response);

    QJsonObject getClientStats(WickrBotClients *client);
    void getStatistics(HttpResponse& response);
    void getStatistics(const QString& clientName, HttpResponse& response);

    bool pauseClient(WickrBotClients *client);
    bool startClient(WickrBotClients *client);

    bool chkClientsNameExists(const QList<WickrIOClients *> &clients,const QString& name);
    bool chkClientsUserExists(const QList<WickrIOClients *> &clients,const QString& name);
    bool chkClientsInterfaceExists(const QList<WickrIOClients *> &clients,const QString& iface, int port);

    bool addClientData(WickrBotClients *newClient);

    bool sendClientCmd(int port, const QString& cmd);

    void updateSSLSettings();
    void generateNewKey(WickrIOConsoleUser *cUser);
    void sendAuthEmail(WickrIOConsoleUser *consoleUser);

signals:

private slots:

public:

private:
    OperationData *m_operation;
    QSettings *m_settings;
    WickrIOSSLSettings m_sslSettings;
    QString m_appNm;
    WickrBotIPC *m_ipc;

    // Client Message handling values
    bool m_clientMsgSuccess;
    bool m_clientMsgInProcess;

    // Email settings to send authorization messages
    WickrIOEmailSettings m_email;


    typedef enum {
        ReqTypeUser,                    // User request type
        ReqTypeClient,                  // Client request type
        ReqTypeStatistic,               // Statistics request type
    } WickrIOConsoleReqType;

};

#endif // CMDHANDLER_H
