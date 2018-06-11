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
#include "wickrIOClientServer.h"

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
    WickrIOClientServer m_clientServer;
};

#endif // WICKRIOCLIENTSERVERSERVICE_H
