#ifndef PARSEROPERATIONDATA_H
#define PARSEROPERATIONDATA_H

#include <QString>

// Here are the RabbitMQ settings
#define WBSETTINGS_RBTQ_HEADER      "rabbitqueue"
#define WBSETTINGS_RBTQ_HOST        "host"
#define WBSETTINGS_RBTQ_PORT        "port"
#define WBSETTINGS_RBTQ_USER        "user"
#define WBSETTINGS_RBTQ_PASSWORD    "password"
#define WBSETTINGS_RBTQ_EXCHANGE    "exchangename"
#define WBSETTINGS_RBTQ_QUEUE       "queuename"
#define WBSETTINGS_RBTQ_VIRTUALHOST "virtualhost"

// Here are the General settings
#define WBSETTINGS_GEN_HEADER       "basicsettings"
#define WBSETTINGS_GEN_DURATION     "duration"
#define WBSETTINGS_GEN_ATTACHDIR    "attachdir"
#define WBSETTINGS_GEN_DBDIR        "dbdir"
#define WBSETTINGS_GEN_LOG          "log"

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
};

#endif // PARSEROPERATIONDATA_H
