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

#ifdef Q_OS_WIN
#define WBIO_DEFAULT_DBLOCATION         TBD
        dbLocation = QString("%1/%2/%3")
                .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                .arg(WBIO_ORGANIZATION)
                .arg(WBIO_GENERAL_TARGET);
#else
#if defined(WICKR_DEBUG)
#define WBIO_DEFAULT_DBLOCATION         "/opt/WickrIODebug"
#else
#define WBIO_DEFAULT_DBLOCATION         "/opt/WickrIO"
#endif
#endif


/*
 * definitions of the TARGET values. Make sure these are consistent with
 * the TARGET definitions within the .pro files.
 */
#if defined(WICKR_BLACKOUT) && defined(WICKR_DEBUG)
#define WBIO_CLIENT_PROCESS         "WickrIOClientOnPrem"

#elif defined(WICKR_BETA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientBeta"

#elif defined(WICKR_ALPHA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientAlpha"

#elif defined(WICKR_PRODUCTION)
#define WBIO_CLIENT_PROCESS         "WickrIOClient"

#elif defined(WICKR_QA)
#define WBIO_CLIENT_PROCESS         "WickrIOClientQA"

#else
"No WICKR_TARGET defined!!!"
#endif

/*
 * WickrIO IPC commands
 */
#define WBIO_IPCCMDS_PAUSE          "pause"
#define WBIO_IPCCMDS_STOP           "stop"

#define WBIO_IPCMSGS_PASSWORD       "password"
#define WBIO_IPCMSGS_USERSIGNKEY    "usersignkey"
#define WBIO_IPCMSGS_STATE          "state"

class WBIOCommon
{
public:
    WBIOCommon() {}

    static bool makeDirectory(QString dirname);
};


#endif // WBIO_COMMON_H
