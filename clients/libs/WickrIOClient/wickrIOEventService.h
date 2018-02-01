#ifndef WICKRIOEVENTSERVICE_H
#define WICKRIOEVENTSERVICE_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>

#include "common/wickrNetworkUtil.h"
#include "wickrIOServiceBase.h"

#include "user/wickrUser.h"
#include "requests/wickrRequests.h"

#include "wickriodatabase.h"
#include "wickrIOJson.h"
#include "operationdata.h"
#include "wickrIOMessageCounter.h"


// Forward declaration
class WickrIOEventThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOEventService : public WickrIOServiceBase
{
    Q_OBJECT
public:
    explicit WickrIOEventService(OperationData *operation);
    virtual ~WickrIOEventService();

    void shutdown();
    bool isHealthy();

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;
    WickrIOEventThread    *m_ahThread;

    void startThreads(OperationData *operation);
    void stopThreads();

signals:
    void signalShutdown();

public slots:

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#define WICKRIO_AH_INVALID_ID           -1
#define WICKRIO_AH_UPDATE_STATS_SECS    3600
#define WICKRIO_AH_UPDATE_PROCESS_SECS  1

class WickrIOEventThread : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOEventThread(QThread *thread, WickrIOEventService *svc, OperationData *operation);
    ~WickrIOEventThread();

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOEventService    *m_parent;
    bool                    m_running;
    OperationData           *m_operation;
    QTimer                  m_timer;

    int m_timerStatsTicker=0;
    int m_eventsSent;

    void processEvents();
    void initCounts()
    {
        m_eventsSent = 0;
    }

    void logCounts()
    {
        if (m_eventsSent > 0) {
            m_operation->log_handler->log("Messages sent", m_eventsSent);
        }
    }

signals:
    void signalExit();
    void signalNotRunning();

private slots:

public slots:
    void slotTimerExpire();
    void slotShutdown();
};

#endif // WICKRIOEVENTSERVICE_H
