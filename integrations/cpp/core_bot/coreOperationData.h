#ifndef COREOPERATIONDATA_H
#define COREOPERATIONDATA_H

#include <QString>
#include "bot_iface.h"
#include "loghandler.h"
#include "wickriodatabase.h"

// Here are the General settings
#define WBSETTINGS_GEN_HEADER           "basicsettings"
#define WBSETTINGS_GEN_DURATION         "duration"
#define WBSETTINGS_GEN_ATTACHDIR        "attachdir"
#define WBSETTINGS_GEN_DBDIR            "dbdir"
#define WBSETTINGS_GEN_LOG              "log"
#define WBSETTINGS_GEN_CLIENT           "clientname"

class CoreOperationData
{
public:
    CoreOperationData() {
        log_handler = new LogHandler();
    }

    bool    debug = false;
    bool    force = false;

    // Name of the WickrIO Bot
    QString m_botName;

    // This is the interface to the client bot API
    BotIface    *m_botIface=nullptr;

    // Log handling
    LogHandler *log_handler = nullptr;

    WickrIOClientDatabase *m_botDB = nullptr;
};

#endif // COREOPERATIONDATA_H
