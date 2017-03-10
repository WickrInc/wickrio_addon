#ifndef WICKRIOCLIENTSERVERSERVICE_H
#define WICKRIOCLIENTSERVERSERVICE_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTimer>

#include <qtservice/qtservice.h>
#include <operationdata.h>
#include <wickrbotipc.h>

#define UPDATE_STATUS_SECS  60

class WickrIOClientServerService : public QObject, public QtService<QCoreApplication>
{
Q_OBJECT

public:
    WickrIOClientServerService(int argc, char **argv);
    virtual ~WickrIOClientServerService();

protected:
    void start();
    void stop();
    void pause();
    void resume();
    void processCommand(int code);

private:
    void usage();
    void getClients(bool start);
    bool clientNeedsStart(QString name, bool isEnterprise=false);
    bool startClient(WickrBotClients *client);
    bool stopClient(const WickrBotProcessState& state);

    bool configureService();

private:
    OperationData *m_operation;
    QTimer *m_processTimer;
    QString m_vendorName;
    int m_statusCntDwn;
    bool m_isConfigured;

    QString m_appNm;

private slots:
    void slotTimeoutProcess();
};

#endif // WICKRIOCLIENTSERVERSERVICE_H
