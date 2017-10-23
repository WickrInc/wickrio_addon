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

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    WickrServiceState       m_state;
    QThread                 m_thread;
    WickrIOWatchdogThread   *m_wdThread;

    void startThreads();
    void stopThreads();

signals:
    void signalShutdown();
    void signalRegisterService(WickrIOServiceBase *svc);
    void signalDeRegisterService(WickrIOServiceBase *svc);

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
    bool                    m_running;
    OperationData           *m_operation;

    QTimer                  m_timer;

    void doStatusUpdate(int state, bool force);

signals:
    void signalNotRunning();

public slots:
    void slotTimerExpire();
    void slotShutdown();

    void slotRegisterService(WickrIOServiceBase *svc);
    void slotDeRegisterService(WickrIOServiceBase *svc);

};

#endif // WICKRIOWATCHDOGSERVICE_H
