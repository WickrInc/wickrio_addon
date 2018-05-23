#include "wickrIOClientRuntime.h"

/**
 * @brief WickrIOClientRuntime (PRIVATE CONSTRUCTOR)
 * Constructor
 */
WickrIOClientRuntime::WickrIOClientRuntime() {

    // Allocate resources
    m_callbackSvc = new WickrIOCallbackService();
    m_fileDownloadSvc = new WickrIOFileDownloadService();
    m_ipcSvc = new WickrIOIPCService();
    m_watchdogSvc = new WickrIOWatchdogService();
    m_provisionHdlr = new WickrIOProvisionHdlr();

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
        WickrIOClientRuntime::operationData()->log_handler->output(str);
        break;
    case QtFatalMsg:
        WickrIOClientRuntime::operationData()->log_handler->output(str);
//        abort();
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

    if (!operation->log_handler->logGetOutput().isEmpty()) {
        operation->log_handler->log(QString("Redirecting output to %1").arg(operation->log_handler->logGetOutput()));
        qInstallMessageHandler(redirectedOutput);
    }
}

/**
 * @brief shutdown
 * Will shutdown runtime, cleaning up all resources.
 */
void WickrIOClientRuntime::shutdown() {
    WickrIOClientRuntime::get().ipcSvc()->shutdown();

    // Tell the watchdog service to shutdown
    WickrIOClientRuntime::get().wdSvc()->shutdown();

//   return WickrIOClientRuntime::get().cleanupResources();
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

void
WickrIOClientRuntime::cbSvcSetSaveAttachment(bool saveAttachment)
{
    cbSvc()->setSaveAttachments(saveAttachment);
}

/**
 * @brief WickrIO File Download Service API
 */
WickrIOFileDownloadService*
WickrIOClientRuntime::fdSvc() {
    return WickrIOClientRuntime::get().m_fileDownloadSvc;
}

bool
WickrIOClientRuntime::fdSvcDownloadFile(WickrIORxDownloadFile *dload) {
    fdSvc()->downloadFile(dload);
    return true;
}

/**
 * @brief WickrIO Interprocess Communications Service API
 */
WickrIOIPCService*
WickrIOClientRuntime::ipcSvc() {
    return WickrIOClientRuntime::get().m_ipcSvc;
}

bool
WickrIOClientRuntime::startIPC() {
    ipcSvc()->startIPC(operationData());
}



/**
 * @brief WickrIO Watchdog Service API
 */
WickrIOWatchdogService*
WickrIOClientRuntime::wdSvc() {
    return WickrIOClientRuntime::get().m_watchdogSvc;
}

void
WickrIOClientRuntime::wdSetShutdownState(int procState)
{
    WickrIOClientRuntime::get().m_watchdogSvc->setShutdownProcState(procState);
}

/**
 * @brief WickrIO Message Callback Service API
 */
WickrIOProvisionHdlr*
WickrIOClientRuntime::provHdlr() {
    return WickrIOClientRuntime::get().m_provisionHdlr;
}

void
WickrIOClientRuntime::provHdlrBeginOnPrem(const QString username, const QString password, const QString regToken)
{
    provHdlr()->onPremBegin(username, password, regToken);
}

void
WickrIOClientRuntime::provHdlrBeginCloud(const QString &email, const QString password, const QString &inviteCode)
{
    provHdlr()->cloudBegin(email, password, inviteCode);
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
    // Close the dynamic services
    for (WickrIOServiceBase *value : w_dynamicServices.values()) {
        value->deleteLater();
    }
    w_dynamicServices.clear();

    delete m_watchdogSvc;           m_watchdogSvc = nullptr;
    delete m_callbackSvc;           m_callbackSvc = nullptr;
    delete m_fileDownloadSvc;       m_fileDownloadSvc = nullptr;
    m_provisionHdlr->deleteLater(); m_provisionHdlr = nullptr;
    m_initialized = false;
}

OperationData *WickrIOClientRuntime::operationData() {
    return WickrIOClientRuntime::get().m_operation;
}

/**
 * @brief WickrIOClientRuntime::addService
 * Add a new service to the list of dynamic services. Once the service is
 * added to the runtime it is assumed that the runtime will delete the service
 * when the runtime is shutdown.
 * @param newSvc
 * @return
 */
bool
WickrIOClientRuntime::addService(WickrIOServiceBase *newSvc)
{
    WickrIOClientRuntime& me = WickrIOClientRuntime::get();

    QString svcName = newSvc->serviceName();
    // if the service exists already then return failed
    if (me.w_dynamicServices.contains(svcName)) {
        qDebug() << "Cannot add service" << svcName << "it aleady exists!";
        return false;
    }
    me.w_dynamicServices.insert(svcName, newSvc);
    return true;
}

/**
 * @brief WickrIOClientRuntime::findService
 * Return the service associated with the input service name string.
 * @param svcName
 * @return
 */
WickrIOServiceBase *
WickrIOClientRuntime::findService(const QString& svcName)
{
    WickrIOClientRuntime& me = WickrIOClientRuntime::get();

    WickrIOServiceBase *retSvc = nullptr;
    if (me.w_dynamicServices.contains(svcName)) {
        retSvc = me.w_dynamicServices.value(svcName, nullptr);
    } else {
        qDebug() << "Cannot find service" << svcName;
    }
    return retSvc;
}

