#ifndef CLIENTCONFIGURATIONINFO_H
#define CLIENTCONFIGURATIONINFO_H

#include <QString>

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_TARGET          "test_botOnPrem"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrOnPrem"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleOnPrem"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_TARGET          "test_botBeta"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrBeta"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleBeta"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_TARGET          "test_botAlpha"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrAlpha"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleAlpha"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_TARGET          "test_bot"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvr"
#define WBIO_CONSOLE_TARGET         "WickrIOConsole"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvr"

#elif defined(WICKR_QA)
#define WBIO_CLIENT_TARGET          "test_botQA"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrQA"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleQA"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrQA"

#else
"No WICKR_TARGET defined!!!"
#endif


class ClientConfigurationInfo {
public:
    static const QString BugsnagURL;
    static const QString BugsnagAPI;

    static const QString DefaultNPLUserSuffix;

    static const QString DefaultBaseURL;

#ifdef WICKR_SPARKLE_ENABLED
    static const QString SparkleAutoUpdateURL;
#endif
};


#endif // CLIENTCONFIGURATIONINFO_H
