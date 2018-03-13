#ifndef WICKRIOCLIENTRUNTIME_H
#define WICKRIOCLIENTRUNTIME_H

#include "operationdata.h"
#include "wickrIOProvisionHdlr.h"

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
    static void init();
    static void shutdown();

    /**
     * @brief provHdlr
     * @return
     */
    static WickrIOProvisionHdlr* provHdlr();
    static void provHdlrBeginOnPrem(const QString username, const QString password, const QString regToken);
    static void provHdlrBeginCloud(const QString &email, const QString password, const QString &inviteCode);

private:
    // Runtime resources
    bool                    m_initialized;

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
     * Will return refereWickrIOProvisionHdlrnce to singleton instance. Instantiated on first use (recommended from init() in main.cpp).
     * Guaranteed to be destroyed.
     * @return
     */
    static WickrIOClientRuntime& get();

    /**
     * @brief m_provisionHdlr
     */
    WickrIOProvisionHdlr *m_provisionHdlr;

    Q_DISABLE_COPY(WickrIOClientRuntime)
};

#endif // WICKRIOCLIENTRUNTIME_H
