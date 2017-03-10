#ifndef WICKRBOTCLIENTS_H
#define WICKRBOTCLIENTS_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotClients
{
public:
    WickrBotClients() : id(0) {};
    ~WickrBotClients() {};

public:
    int id;

    QString name;
    QString apiKey;
    QString user;
    QString password;
    QString status;

    bool isHttps;
    int port;
    QString iface;
    QString sslKeyFile;
    QString sslCertFile;

public:
    QString getIfaceTypeStr() { return isHttps ? QString("HTTPS") : QString("HTTP"); }
    bool isEnterprise() {
        if (!apiKey.isEmpty() && apiKey.toLower().startsWith("e"))
            return true;
        return false;
    }
};

#endif
