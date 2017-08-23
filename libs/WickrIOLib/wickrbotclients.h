#ifndef WICKRBOTCLIENTS_H
#define WICKRBOTCLIENTS_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotClients
{
public:
    WickrBotClients() :
        id(0),
        m_handleInbox(false),
        onPrem(false)
    {};

    ~WickrBotClients() {};

public:
    int id;

    QString name;
    QString apiKey;
    QString user;
    QString password;
    QString status;
    QString binary;

    bool isHttps;
    int port;
    QString iface;
    QString sslKeyFile;
    QString sslCertFile;

    bool    m_handleInbox;      // true if client should handle inbox messages, depends on client
    bool    onPrem;

public:
    QString getIfaceTypeStr() { return isHttps ? QString("HTTPS") : QString("HTTP"); }
    QString getHandleInboxStr() { return m_handleInbox ? QString("true") : QString("false"); }
};

#endif
