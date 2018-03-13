#ifndef WICKRBOTCLIENTS_H
#define WICKRBOTCLIENTS_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotClients
{
public:
    WickrBotClients() {};

    ~WickrBotClients() {};

public:
    int id=0;

    QString name;
    QString apiKey;
    QString user;
    QString password;
    QString transactionID;
    QString status;
    QString binary;

    bool isHttps;
    int port = 0;
    QString iface;
    QString sslKeyFile;
    QString sslCertFile;

    bool    m_handleInbox=false; // true if client should handle inbox messages, depends on client
    bool    onPrem=false;

    int console_id=0;           // ID of the associated Console User or 0

public:
    QString getIfaceTypeStr() { return isHttps ? QString("HTTPS") : QString("HTTP"); }
    QString getHandleInboxStr() { return m_handleInbox ? QString("true") : QString("false"); }

    QString getProcessName() { return(QString("%1.%2").arg(binary).arg(name)); }
};

#endif
