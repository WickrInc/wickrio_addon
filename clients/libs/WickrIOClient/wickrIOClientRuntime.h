#ifndef WICKRIOCLIENTRUNTIME_H
#define WICKRIOCLIENTRUNTIME_H

#include "wickrIOCallbackService.h"
#include "wickrIOFileDownloadService.h"
#include "wickrIOWatchdogService.h"

#include "operationdata.h"

// Forward declaration
class WickrIOServiceBase;

/**
 * @brief The WickrIOClientRuntime class
 *
 */
class WickrIOClientRuntime
{

public:
    // Destructor
    virtual ~WickrIOClientRuntime();

    // Runtime Init/Shutdown API
    static void init(OperationData *operation);
    static void shutdown();

    // Dynamic service functions
    bool addService(WickrIOServiceBase *newSvc);
    WickrIOServiceBase *findService(const QString& svcName);

    /**
     * Message Callback Service API
     */
    static WickrIOCallbackService* cbSvc();
    static bool cbSvcMessagesPending();

    /**
     * File Download Service API
     */
    static WickrIOFileDownloadService *fdSvc();
    static bool fdSvcDownloadFile(WickrIORxDownloadFile *dload);

    /**
     * Watchdog Service API
     */
    static WickrIOWatchdogService* wdSvc();


    // Component accessors
    static OperationData *operationData();

    static void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str);

private:
    // Runtime resources
    bool                    m_initialized;
    OperationData           *m_operation;

    WickrIOCallbackService      *m_callbackSvc;
    WickrIOFileDownloadService  *m_fileDownloadSvc;
    WickrIOWatchdogService      *m_watchdogSvc;

    // Map service names to dynamic services
    QMap<QString, WickrIOServiceBase *> w_dynamicServices;

    /**
     * @brief WickrIOClientRuntime (PRIVATE CONSTRUCTOR)
     * Constructor
     */
    WickrIOClientRuntime();

    /**
     * @brief cleanup
     * Cleanup all runtime resources
     */
    void cleanupResources();

    /**
     * @brief get
     * Will return reference to singleton instance. Instantiated on first use (recommended from init() in main.cpp).
     * Guaranteed to be destroyed.
     * @return
     */
    static WickrIOClientRuntime& get();

    Q_DISABLE_COPY(WickrIOClientRuntime)
};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOServiceBase : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOServiceBase(const QString& serviceName);
    virtual ~WickrIOServiceBase() {};

    QString serviceName() { return m_serviceName; }

protected:
    WickrServiceState   m_state;        // state of the service
    QThread             m_thread;       // thread associated with the service
    long                m_heartbeat;    // heartbeat of the service, for keep alives

private:
    QString             m_serviceName;  // Unique name of this service
};

#endif // WICKRIOCLIENTRUNTIME_H
