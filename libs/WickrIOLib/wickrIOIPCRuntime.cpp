#include "wickrIOIPCRuntime.h"

/**
 * @brief WickrIOIPCRuntime (PRIVATE CONSTRUCTOR)
 * Constructor
 */
WickrIOIPCRuntime::WickrIOIPCRuntime() {
}

/**
 * @brief ~WickrIOIPCRuntime
 * Destructor
 */
WickrIOIPCRuntime::~WickrIOIPCRuntime() {
    if (m_initialized)
        cleanupResources();
}

void
WickrIOIPCRuntime::setOperationData(OperationData *operation)
{
    m_operation = operation;
}

void
WickrIOIPCRuntime::setEndpointInfo(const QString& name, bool isClient) {
    m_isClient = isClient;
    m_name = name;

    if (m_ipcSvc == nullptr) {
        m_ipcSvc = new WickrIOIPCService(name, isClient);
        m_ipcSvc->startIPC(WickrIOIPCRuntime::operationData());
    }
    m_initialized = true;
}

/**
 * @brief init
 * Will initialize runtime singleton via get(). Should be called in main.cpp.
 * @param isDebug    - set debug mode for use by request context classes.
 */
void
WickrIOIPCRuntime::init(OperationData *operation) {
    // Instantiate runtime
    WickrIOIPCRuntime& me = WickrIOIPCRuntime::get();
    me.setOperationData(operation);
    me.setEndpointInfo(operation->wickrID, true);
}

/**
 * @brief WickrIOIPCRuntime::init
 * Overloaded init function
 * @param name
 * @param isClient
 */
void
WickrIOIPCRuntime::init(const QString& name, bool isClient) {
    // Instantiate runtime
    WickrIOIPCRuntime& me = WickrIOIPCRuntime::get();
    me.setEndpointInfo(name, isClient);
}

/**
 * @brief shutdown
 * Will shutdown runtime, cleaning up all resources.
 */
void WickrIOIPCRuntime::shutdown() {
    WickrIOIPCRuntime::get().ipcSvc()->shutdown();

   WickrIOIPCRuntime::get().cleanupResources();
}

/**
 * @brief get (PRIVATE STATIC)
 * Will return reference to singleton instance. Instantiated on first use (recommended from init() in main.cpp).
 * Guaranteed to be destroyed.
 * @return
 */
WickrIOIPCRuntime& WickrIOIPCRuntime::get() {
    static WickrIOIPCRuntime instance;
    return instance;
}

/**
 * @brief cleanup (PRIVATE)
 * Cleanup all runtime resources
 */
void WickrIOIPCRuntime::cleanupResources() {
    m_initialized = false;
}

OperationData *
WickrIOIPCRuntime::operationData() {
    return WickrIOIPCRuntime::get().m_operation;
}

QString
WickrIOIPCRuntime::name() {
    return WickrIOIPCRuntime::get().m_name;
}


/**
 * @brief WickrIO Interprocess Communications Service API
 */
WickrIOIPCService*
WickrIOIPCRuntime::ipcSvc() {
    return WickrIOIPCRuntime::get().m_ipcSvc;
}

bool
WickrIOIPCRuntime::startIPC() {
    ipcSvc()->startIPC(operationData());
}



