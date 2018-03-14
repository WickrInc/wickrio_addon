#ifndef WICKRIOWATCHDOGSERVICE_H
#define WICKRIOWATCHDOGSERVICE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include "common/wickrNetworkUtil.h"
#include "wickrIOServiceBase.h"
#include "operationdata.h"

// Forward declaration
class WickrIOWatchdogThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOWatchdogService : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOWatchdogService();
    virtual ~WickrIOWatchdogService();

    void shutdown();

    void registerService(WickrIOServiceBase *svc);
    void deRegisterService(WickrIOServiceBase *svc);

    // Set the process state to set when shutting down
    void setShutdownProcState(int procState) { m_shutdownMode = procState; }

    // Set the logged in state
    void setLoggedIn(bool loggedIn) { emit signalLoggedIn(loggedIn); }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    WickrServiceState       m_state;
    QThread                 m_thread;
    WickrIOWatchdogThread   *m_wdThread;
    int                     m_shutdownMode;

    void startThreads();
    void stopThreads();

signals:
    void signalShutdown(int procState);
    void signalRegisterService(WickrIOServiceBase *svc);
    void signalDeRegisterService(WickrIOServiceBase *svc);
    void signalServiceNotLoggedIn();
    void signalLoggedIn(bool loggedIn);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#define WICKRIO_WATCHDOG_UPDATE_PROCESS_SECS    60

class WickrIOWatchdogThread : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOWatchdogThread(QThread *thread, WickrIOWatchdogService *svc);
    virtual ~WickrIOWatchdogThread();

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOWatchdogService  *m_parent;
    bool                    m_running = false;
    bool                    m_loggedIn = false;
    OperationData           *m_operation;

    QTimer                  m_timer;

    void doStatusUpdate(int state, bool force);

signals:
    void signalNotRunning();
    void signalServiceNotLoggedIn();    // Emitted when a service is not logged in for a period of time

public slots:
    void slotTimerExpire();
    void slotShutdown(int procState);

    void slotRegisterService(WickrIOServiceBase *svc);
    void slotDeRegisterService(WickrIOServiceBase *svc);

    void slotLoggedIn(bool loggedIn);

};

#endif // WICKRIOWATCHDOGSERVICE_H
