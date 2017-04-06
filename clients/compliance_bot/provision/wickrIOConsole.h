#ifndef WICKRIOCONSOLE_H
#define WICKRIOCONSOLE_H

#include <QObject>
#include <QThread>
#include <QCoreApplication>
#include "operationdata.h"
#include "wickrioipc.h"
#include "cmdMain.h"

// Forward declaration
class WickrIOConsoleThread;

class WickrIOConsoleService : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOConsoleService(QCoreApplication *app, int argc, char **argv, OperationData *operation, WickrBotMainIPC *rxIPC);
    virtual ~WickrIOConsoleService();

    void startConsole();

    QCoreApplication    *m_app;
    int                 m_argc;
    char                **m_argv;
    OperationData       *m_operation;
    WickrBotMainIPC     *m_rxIPC;

private:
    QThread                m_thread;
    WickrIOConsoleThread   *m_consoleThread;

    void startThreads();
    void stopThreads();

signals:
    void signalStartConsole();

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOConsoleThread : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOConsoleThread(QThread *thread, WickrIOConsoleService *svc);
    virtual ~WickrIOConsoleThread();

private:
    WickrIOConsoleService   *m_parent;
    bool                    m_running;

public slots:
    void slotStartConsole();

};




#endif // WICKRIOCONSOLE_H
