#ifndef WICKRIOAPPSETTINGS_H
#define WICKRIOAPPSETTINGS_H

#include <QDateTime>
#include <QString>
#include <QSettings>
#include "wickrbotlib.h"

/**
 * @brief The WickrIOEmailSettings class
 * This class contains the email settings values. Use the getEmail() function of the
 * WickrIOAppSettings to populate this class.
 */
class WickrIOEmailSettings //DECLSPEC
{
public:
    WickrIOEmailSettings() {}
    ~WickrIOEmailSettings() {}

    bool readFromSettings(QSettings *settings);
    bool saveToSettings(QSettings *settings);

public:
    QString server;
    int port;
    QString type;
    QString account;
    QString password;
    QString sender;
    QString recipient;
    QString subject;
};

class WickrIOSSLSettings //DECLSPEC
{
public:
    WickrIOSSLSettings() {}
    ~WickrIOSSLSettings() {}

    bool readFromSettings(QSettings *settings);
    bool saveToSettings(QSettings *settings);

    bool validateSSLKey() { return validateSSLKey(sslKeyFile); }
    bool validateSSLKey(const QString &fileName);
    bool validateSSLCert() { return validateSSLCert(sslCertFile); }
    bool validateSSLCert(const QString &fileName);

public:
    QString sslKeyFile;
    QString sslCertFile;
};

/**
 * @brief The WickrIOAppSettings class
 * This is the in memory class to contain the application values.
 */
class WickrIOAppSettings //DECLSPEC
{
public:
    WickrIOAppSettings() {}
    ~WickrIOAppSettings() {}

    bool setupEmail(QString server, int port, QString type, QString account, QString password,
                    QString sender, QString recipient, QString subject);
    bool getEmail(WickrIOEmailSettings *email);

public:
    int id;
    int clientID;
    QString type;
    QString value;
};

#endif // WICKRIOAPPSETTINGS_H

