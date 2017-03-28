#include "wickrIOClientRuntime.h"

/**
 * @brief WickrIOClientRuntime (PRIVATE CONSTRUCTOR)
 * Constructor
 */
WickrIOClientRuntime::WickrIOClientRuntime() {

    // Allocate resources
    m_callbackSvc = new WickrIOCallbackService();

    m_initialized = true;
}

/**
 * @brief ~WickrIOClientRuntime
 * Destructor
 */
WickrIOClientRuntime::~WickrIOClientRuntime() {
    if (m_initialized)
        cleanupResources();
}

void
WickrIOClientRuntime::redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        break;
    case QtFatalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        abort();
    }
}

/**
 * @brief init
 * Will initialize runtime singleton via get(). Should be called in main.cpp.
 * @param isDebug    - set debug mode for use by request context classes.
 */
void WickrIOClientRuntime::init(OperationData *operation) {
    // Instantiate runtime
    WickrIOClientRuntime& me = WickrIOClientRuntime::get();
    me.m_operation = operation;

    if (!operation->logGetOutput().isEmpty()) {
        operation->log(QString("Redirecting output to %1").arg(operation->logGetOutput()));
        qInstallMessageHandler(redirectedOutput);
    }
}

/**
 * @brief shutdown
 * Will shutdown runtime, cleaning up all resources.
 */
void WickrIOClientRuntime::shutdown() {
   return WickrIOClientRuntime::get().cleanupResources();
}

/**
 * @brief WickrIO Message Callback Service API
 */
WickrIOCallbackService*
WickrIOClientRuntime::cbSvc() {
    return WickrIOClientRuntime::get().m_callbackSvc;
}

bool
WickrIOClientRuntime::cbSvcMessagesPending() {
    cbSvc()->messagesPending();
    return true;
}

/**
 * @brief get (PRIVATE STATIC)
 * Will return reference to singleton instance. Instantiated on first use (recommended from init() in main.cpp).
 * Guaranteed to be destroyed.
 * @return
 */
WickrIOClientRuntime& WickrIOClientRuntime::get() {
    static WickrIOClientRuntime instance;
    return instance;
}

/**
 * @brief cleanup (PRIVATE)
 * Cleanup all runtime resources
 */
void WickrIOClientRuntime::cleanupResources() {
    delete m_callbackSvc;   m_callbackSvc = nullptr;
    m_initialized = false;
}

OperationData *WickrIOClientRuntime::operationData() {
    return WickrIOClientRuntime::get().m_operation;
}
