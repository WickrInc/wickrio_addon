#ifndef WICKRIOCLIENTSERVERPROCESS_H
#define WICKRIOCLIENTSERVERPROCESS_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QTimer>

#include <operationdata.h>
#include <wickrbotipc.h>
#include "wickrIOIPCService.h"
#include "wickrIOClientServer.h"

#define WICKRIOCLIENTSERVERPROCESS WickrIOClientServerProcess::theClientServer

class WickrIOClientServerProcess : public QThread
{
Q_OBJECT

public:
    WickrIOClientServerProcess(OperationData *pOperation=nullptr);
    virtual ~WickrIOClientServerProcess();

    static WickrIOClientServerProcess *theClientServer;

private:
    WickrIOClientServer m_clientServer;

private slots:
    void processStarted();
    void processFinished();
};

#endif // WICKRIOCLIENTSERVERPROCESS_H
