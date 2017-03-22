#ifndef WBIO_COMMON_H
#define WBIO_COMMON_H

#include <QString>
#include <QSettings>

/*
 * Definitions of common values used throughout the different
 * WickrIO applications.
 */

#define WBIO_ORGANIZATION       "Wickr, LLC"

#ifdef Q_OS_WIN
#define WBIO_SERVER_SETTINGS_FORMAT     "HKEY_LOCAL_MACHINE\\SOFTWARE\\%1\\%2"
#define WBIO_CLIENT_SETTINGS_FORMAT     "HKEY_LOCAL_MACHINE\\SOFTWARE\\%1\\%2\\clients\\%3"
#else
#define WBIO_CLIENT_SETTINGS_FORMAT     "%1/clients/%2/WickrIOClient.%2.ini"
#endif
#define WBIO_CLIENT_DBDIR_FORMAT        "%1/clients/%2/client"
#define WBIO_CLIENT_LOGFILE_FORMAT      "%1/clients/%2/logs/WickrIO%2.log"
#define WBIO_CLIENT_OUTFILE_FORMAT      "%1/clients/%2/logs/WickrIO%2.output"
#define WBIO_CLIENT_WORKINGDIR_FORMAT   "%1/clients/%2"
#define WBIO_CLIENT_ATTACHDIR_FORMAT    "%1/clients/%2/attachments"

/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BETA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientBeta"
#define WBIO_ECLIENT_TARGET         "WickrIOEClientBeta"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrBeta"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleBeta"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrBeta"
#define WBIO_GENERAL_TARGET         "WickrIOBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientAlpha"
#define WBIO_ECLIENT_TARGET         "WickrIOEClientAlpha"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrAlpha"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleAlpha"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrAlpha"
#define WBIO_GENERAL_TARGET         "WickrIOAlpha"

#elif defined(WICKR_PROD)
#define WBIO_CLIENT_PROCESS         "WickrIOClient"
#define WBIO_ECLIENT_TARGET         "WickrIOEClient"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvr"
#define WBIO_CONSOLE_TARGET         "WickrIOConsole"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvr"
#define WBIO_GENERAL_TARGET         "WickrIO"

#elif defined(WICKR_QA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientQA"
#define WBIO_ECLIENT_TARGET         "WickrIOEClientQA"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrQA"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleQA"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrQA"
#define WBIO_GENERAL_TARGET         "WickrIOQA"

#else
"No WICKR_TARGET defined!!!"
#endif

/*
 * WickrIO IPC commands
 */
#define WBIO_IPCCMDS_PAUSE          "pause"
#define WBIO_IPCCMDS_STOP           "stop"

class WBIOCommon
{
public:
    WBIOCommon() {}

    static bool makeDirectory(QString dirname);
    static QSettings *getSettings();
    static QString getDBLocation();
    static QString getClientProcessName(QString name);
};


#endif // WBIO_COMMON_H
