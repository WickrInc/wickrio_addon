#ifndef WICKRIOCLIENTRUNTIME_H
#define WICKRIOCLIENTRUNTIME_H

#include <QObject>

#include "wickrIOServiceBase.h"
#include "wickrIOCallbackService.h"
#include "wickrIOFileDownloadService.h"
#include "wickrIOIPCService.h"
#include "wickrIOWatchdogService.h"
#include "wickrIOProvisionHdlr.h"

#include "operationdata.h"

// Forward declaration
class WickrIOServiceBase;

// Define optional services to start
#define WIO_RUNTIME_JAVASCRIPT_SVC  0x01

/**
 * @brief The WickrIOClientRuntime class
 *
 */
class WickrIOClientRuntime : public QObject
{
    Q_OBJECT

public:
    // Destructor
    virtual ~WickrIOClientRuntime();

    // method to pass the operation data to the singleton
    void setOperationData(OperationData *operation);

    // Runtime Init/Shutdown API
    static void init(OperationData *operation);
    static void shutdown();

    // Dynamic service functions
    static bool addService(WickrIOServiceBase *newSvc);
    static WickrIOServiceBase *findService(const QString& svcName);

    /**
     * Message Callback Service API
     */
    static WickrIOCallbackService* cbSvc();
    static bool cbSvcMessagesPending();
    static void cbSvcSetSaveAttachment(bool saveAttachment);

    /**
     * File Download Service API
     */
    static WickrIOFileDownloadService *fdSvc();
    static bool fdSvcDownloadFile(WickrIORxDownloadFile *dload);

    /**
     * IPC Service API
     */
    static WickrIOIPCService *ipcSvc();
    static bool startIPC();

    /**
     * Watchdog Service API
     */
    static WickrIOWatchdogService* wdSvc();
    static void wdSetShutdownState(int procState);

    /**
     * @brief provHdlr
     * @return
     */
    static WickrIOProvisionHdlr* provHdlr();
    static void provHdlrBeginOnPrem(const QString username, const QString password, const QString regToken);
    static void provHdlrBeginCloud(const QString &email, const QString password, const QString &inviteCode);


    /*
     * cleanup of sending files, remove encrypted files
     */
    static bool getFileSendCleanup() { return get().m_removeSentEncryptedFiles; }
    static void setFileSendCleanup(bool flag) { get().m_removeSentEncryptedFiles = flag; }

    // Component accessors
    static OperationData *operationData();

    static void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str);

private:
    // Runtime resources
    bool                    m_initialized;
    OperationData           *m_operation;

    WickrIOCallbackService      *m_callbackSvc = nullptr;
    WickrIOFileDownloadService  *m_fileDownloadSvc = nullptr;
    WickrIOIPCService           *m_ipcSvc = nullptr;
    WickrIOWatchdogService      *m_watchdogSvc = nullptr;
    WickrIOProvisionHdlr        *m_provisionHdlr = nullptr;

    // Client runtime flags
    bool    m_removeSentEncryptedFiles = false;

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

#endif // WICKRIOCLIENTRUNTIME_H
