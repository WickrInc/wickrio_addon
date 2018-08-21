#include "wickrIOClientRuntime.h"

/**
 * @brief WickrIOClientRuntime (PRIVATE CONSTRUCTOR)
 * Constructor
 */
WickrIOClientRuntime::WickrIOClientRuntime() {

    // Allocate resources
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

/**
 * @brief init
 * Will initialize runtime singleton via get(). Should be called in main.cpp.
 * @param isDebug    - set debug mode for use by request context classes.
 */
void WickrIOClientRuntime::init() {
    // Instantiate runtime
    WickrIOClientRuntime& me = WickrIOClientRuntime::get();
}

/**
 * @brief shutdown
 * Will shutdown runtime, cleaning up all resources.
 */
void WickrIOClientRuntime::shutdown() {
   return WickrIOClientRuntime::get().cleanupResources();
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
    m_provisionHdlr->deleteLater();
    m_provisionHdlr = nullptr;

    m_initialized = false;
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
WickrIOClientRuntime::provHdlrBeginCloud(const QString &username, const QString password, const QString &inviteCode)
{
    provHdlr()->cloudBegin(username, password, inviteCode);
}
