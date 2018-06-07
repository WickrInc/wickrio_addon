#ifndef WICKRIOCLIENTSERVER_H
#define WICKRIOCLIENTSERVER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QTimer>

#include <operationdata.h>
#include <wickrbotipc.h>
#include "wickrIOIPCService.h"

#define UPDATE_STATUS_SECS  60
#define BACK_OFF_START      1
#define BACK_OFF_MAX        60

#define WICKRIOCLIENTSERVER WickrIOClientServer::theClientServer

class WickrIOClientServer : public QThread
{
Q_OBJECT

public:
    WickrIOClientServer();
    virtual ~WickrIOClientServer();

    static WickrIOClientServer *theClientServer;

private:
    void usage();
    bool getClients(bool start);
    bool clientNeedsStart(WickrBotClients *client);
    bool parserNeedsStart(WickrBotProcessState *process);
    bool startClient(WickrBotClients *client);
    bool stopClient(const WickrBotProcessState& state);
    bool startParser(QString processName, QString appName);

    bool sendClientCmd(int port, const QString& cmd);

    bool configureService();

    OperationData       *m_operation = nullptr;
    QTimer              *m_processTimer = nullptr;
    QString             m_vendorName;
    int                 m_statusCntDwn;
    int                 m_backOffCntDwn;
    int                 m_backOff;
    bool                m_isConfigured = false;

    QString             m_appNm;
    WickrIOIPCService   *m_ipcSvc = nullptr;

    QString             m_clientState;
    bool                m_clientStateChanged;

    QMap<QString, QString>  m_clientPasswords;

private slots:
    void processStarted();
    void processFinished();

    void slotTimeoutProcess();
    void slotRxIPCMessage(QString type, QString value);

};

#endif // WICKRIOCLIENTSERVER_H
