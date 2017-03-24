#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include <QString>
#include <QSettings>

#include "wickrbotclients.h"

#if defined(WICKR_DEBUG)
#define WBIO_GENERAL_TARGET         "WickrIODebug"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvrDebug"
#define WBIO_CONSOLE_TARGET         "WickrIOConsoleDebug"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvrDebug"
#else
#define WBIO_GENERAL_TARGET         "WickrIO"
#define WBIO_CLIENTSERVER_TARGET    "WickrIOSvr"
#define WBIO_CONSOLE_TARGET         "WickrIOConsole"
#define WBIO_CONSOLESERVER_TARGET   "WickrIOCSvr"
#endif

class WBIOServerCommon
{
public:
    WBIOServerCommon() {}

    static QSettings *getSettings();
    static QString getDBLocation();

    static QString getClientProcessName(WickrBotClients *client);
};


#endif // SERVER_COMMON_H
