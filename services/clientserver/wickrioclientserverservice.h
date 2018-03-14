#ifndef WICKRIOCLIENTSERVERSERVICE_H
#define WICKRIOCLIENTSERVERSERVICE_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTimer>

#include <qtservice/qtservice.h>
#include <operationdata.h>
#include <wickrbotipc.h>
#include "wickrIOIPCService.h"

#define UPDATE_STATUS_SECS  60
#define BACK_OFF_START      1
#define BACK_OFF_MAX        60

class WickrIOClientServerService : public QObject, public QtService<QCoreApplication>
{
Q_OBJECT

public:
    WickrIOClientServerService(int argc, char **argv);
    virtual ~WickrIOClientServerService();

    void start();
    void stop();
    void pause();
    void resume();
    void processCommand(int code);

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

private:
    OperationData       *m_operation;
    QTimer              *m_processTimer;
    QString             m_vendorName;
    int                 m_statusCntDwn;
    int                 m_backOffCntDwn;
    int                 m_backOff;
    bool                m_isConfigured;

    QString             m_appNm;
    WickrIOIPCService   *m_ipcSvc;

    QString             m_clientState;
    bool                m_clientStateChanged;

    QMap<QString, QString>  m_clientPasswords;

private slots:
    void slotTimeoutProcess();
    void slotRxIPCMessage(QString type, QString value);

};

#endif // WICKRIOCLIENTSERVERSERVICE_H
