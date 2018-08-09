#ifndef WICKRIOIPCRUNTIME_H
#define WICKRIOIPCRUNTIME_H

#include <QObject>
#include "wickrIOIPCService.h"

class WickrIOIPCRuntime : public QObject
{
    Q_OBJECT
public:
    // Destructor
    virtual ~WickrIOIPCRuntime();

    // method to pass the operation data to the singleton
    void setOperationData(OperationData *operation);

    // Runtime Init/Shutdown API
    static void init(OperationData *operation);
    static void init(const QString& name, bool isClient);

    static void shutdown();

    /**
     * IPC Service API
     */
    static WickrIOIPCService *ipcSvc();
    static bool startIPC();

    // Component accessors
    static OperationData *operationData();

private:
    // Runtime resources
    bool                m_initialized = false;
    QString             m_name;
    bool                m_isClient = false;

    WickrIOIPCService   *m_ipcSvc = nullptr;
    OperationData       *m_operation = nullptr;

    void setEndpointInfo(const QString& name, bool isClient);

    /**
     * @brief WickrIOIPCRuntime (PRIVATE CONSTRUCTOR)
     * Constructor
     */
    WickrIOIPCRuntime();

    /**
     * @brief cleanup
     * Cleanup all runtime resources
     */
    void cleanupResources();

    /**
     * @brief get
     * Will return refereWickrIOProvisionHdlrnce to singleton instance. Instantiated on first use (recommended from init() in main.cpp).
     * Guaranteed to be destroyed.
     * @return
     */
    static WickrIOIPCRuntime& get();

    Q_DISABLE_COPY(WickrIOIPCRuntime)

};

#endif // WICKRIOIPCRUNTIME_H
