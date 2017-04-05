#ifndef WICKRIOCLIENTRUNTIME_H
#define WICKRIOCLIENTRUNTIME_H

#include "wickrIOCallbackService.h"
#include "wickrIOFileDownloadService.h"

#include "operationdata.h"


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

    // Component accessors
    static OperationData *operationData();

    static void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str);

private:
    // Runtime resources
    bool                    m_initialized;
    OperationData           *m_operation;

    WickrIOCallbackService      *m_callbackSvc;
    WickrIOFileDownloadService  *m_fileDownloadSvc;


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

#endif // WICKRIOCLIENTRUNTIME_H
