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

class WickrIOClientServer : public QObject
{
Q_OBJECT

public:
    WickrIOClientServer(QObject *parent=0);
    virtual ~WickrIOClientServer();

    bool configureService();

    bool getClients(bool start);
    bool clientNeedsStart(WickrBotClients *client);
    bool parserNeedsStart(WickrBotProcessState *process);
    bool startClient(WickrBotClients *client);
    bool stopClient(const QString& name);
    bool startParser(QString processName, QString appName);

    bool sendClientCmd(const QString& dest, const QString& cmd);

    OperationData       *m_operation = nullptr;
    QTimer              *m_processTimer = nullptr;
    QString             m_vendorName;
    int                 m_statusCntDwn;
    int                 m_backOffCntDwn;
    int                 m_backOff;
    bool                m_isConfigured = false;

    WickrIOIPCService   *m_ipcSvc = nullptr;

    QString             m_clientState;
    bool                m_clientStateChanged;

    QMap<QString, QString>  m_clientPasswords;

public slots:
    void processStarted(bool resume=false);
    void processFinished(bool pause=false);

private slots:
    void slotTimeoutProcess();
    void slotRxIPCMessage(QString type, QString value);

};

#endif // WICKRIOCLIENTSERVER_H
