#ifndef PARSEROPERATIONDATA_H
#define PARSEROPERATIONDATA_H

#include <QString>
#include "bot_iface.h"

// Here are the RabbitMQ settings
#define WBSETTINGS_RBTQ_HEADER          "rabbitqueue"
#define WBSETTINGS_RBTQ_HOST            "host"
#define WBSETTINGS_RBTQ_PORT            "port"
#define WBSETTINGS_RBTQ_USER            "user"
#define WBSETTINGS_RBTQ_PASSWORD        "password"
#define WBSETTINGS_RBTQ_EXCHANGE        "exchangename"
#define WBSETTINGS_RBTQ_QUEUE           "queuename"
#define WBSETTINGS_RBTQ_VIRTUALHOST     "virtualhost"

// Here are the General settings
#define WBSETTINGS_GEN_HEADER           "basicsettings"
#define WBSETTINGS_GEN_DURATION         "duration"
#define WBSETTINGS_GEN_ATTACHDIR        "attachdir"
#define WBSETTINGS_GEN_DBDIR            "dbdir"
#define WBSETTINGS_GEN_LOG              "log"
#define WBSETTINGS_GEN_CLIENT           "clientname"
#define WBSETTINGS_GEN_WELCOMEUSER_MSG  "welcomeUserMessage"
#define WBSETTINGS_GEN_WELCOMEADMIN_MSG "welcomeAdminMessage"
#define WBSETTINGS_GEN_NEWDEVICE_MSG    "newDeviceMessage"
#define WBSETTINGS_GEN_FORGOTPW_MSG     "forgotPasswordMessage"

class ParserOperationData
{
public:
    ParserOperationData();

    bool    debug = false;
    bool    force = false;

    // Settings for the QAMQP Queue
    QString queueHost;
    int     queuePort;
    QString queueUsername;
    QString queuePassword;
    QString queueExchange;
    QString queueName;
    QString queueVirtualHost;

    // Name of the WickrIO Bot
    QString m_botName;

    // This is the interface to the client bot API
    BotIface    *m_botIface=nullptr;

    // Messages
    QString m_welcomeUserMessage;
    QString m_welcomeAdminMessage;
    QString m_newDeviceMessage;
    QString m_forgotPwMessage;
};

#endif // PARSEROPERATIONDATA_H
