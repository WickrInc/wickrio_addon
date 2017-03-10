#ifndef WICKRIOIPC_H
#define WICKRIOIPC_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickrbotdatabase.h"
#include "operationdata.h"
#include "wickrbotipc.h"

#include "user/wickrUser.h"

#define WICKRBOTIPC WickrBotMainIPC::theBotIPC

class WickrBotMainIPC : public QThread
{
    Q_OBJECT
public:
    WickrBotMainIPC(OperationData *operation);
    ~WickrBotMainIPC();

    bool check();

    static WickrBotMainIPC *theBotIPC;

private:
    OperationData *m_operation;

    WickrBotIPC *m_ipc;

private slots:
    void processStarted();
    void stopAndExit();

signals:
    void signalGotStopRequest();
    void signalGotPauseRequest();
};


#endif // WICKRIOIPC_H
