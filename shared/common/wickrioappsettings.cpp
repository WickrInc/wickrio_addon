#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>

#include "wickrioappsettings.h"
#include "wickrbotsettings.h"

/**
 * @brief WickrIOEmailSettings::readFromSettings
 * Populate the values of the WickrIOEmailSettings class using the input
 * settings file. The input settings file should be at the specific settings
 * group that contains the email settings. For example, a settings->beginGroup()
 * call should be made before calling this function.
 * @param settings The settings object that references the email settings
 * @return True if valid values are read from the settings file
 */
bool
WickrIOEmailSettings::readFromSettings(QSettings *settings)
{
    server = settings->value(WBSETTINGS_EMAIL_SERVER, QString()).toString();
    port = settings->value(WBSETTINGS_EMAIL_PORT, 0).toInt();
    type = settings->value(WBSETTINGS_EMAIL_TYPE, QString()).toString();
    account = settings->value(WBSETTINGS_EMAIL_ACCOUNT, QString()).toString();
    password = settings->value(WBSETTINGS_EMAIL_PASSWORD, QString()).toString();
    sender = settings->value(WBSETTINGS_EMAIL_SENDER, QString()).toString();
    recipient = settings->value(WBSETTINGS_EMAIL_RECIPIENT, QString()).toString();
    subject = settings->value(WBSETTINGS_EMAIL_SUBJECT, QString()).toString();

    if (server.isEmpty() || port <= 0 || type.isEmpty() || account.isEmpty() ||
        password.isEmpty() || sender.isEmpty() || recipient.isEmpty()) {
        return false;
    }
    return true;
}

bool
WickrIOEmailSettings::saveToSettings(QSettings *settings)
{
    settings->setValue(WBSETTINGS_EMAIL_SERVER, server);
    settings->setValue(WBSETTINGS_EMAIL_PORT, port);
    settings->setValue(WBSETTINGS_EMAIL_TYPE, type);
    settings->setValue(WBSETTINGS_EMAIL_ACCOUNT, account);
    settings->setValue(WBSETTINGS_EMAIL_PASSWORD, password);
    settings->setValue(WBSETTINGS_EMAIL_SENDER, sender);
    settings->setValue(WBSETTINGS_EMAIL_RECIPIENT, recipient);
    if (!subject.isEmpty()) {
        settings->setValue(WBSETTINGS_EMAIL_SUBJECT, subject);
    }
    settings->sync();
    return true;
}

bool
WickrIOSSLSettings::readFromSettings(QSettings *settings)
{
    sslKeyFile = settings->value(WBSETTINGS_SSL_KEYFILE, QString()).toString();
    sslCertFile = settings->value(WBSETTINGS_SSL_CERTFILE, QString()).toString();

    if (sslKeyFile.isEmpty() || sslCertFile.isEmpty()) {
        return false;
    }
    return true;
}

bool
WickrIOSSLSettings::saveToSettings(QSettings *settings)
{
    settings->setValue(WBSETTINGS_SSL_KEYFILE, sslKeyFile);
    settings->setValue(WBSETTINGS_SSL_CERTFILE, sslCertFile);
    settings->sync();
    return true;
}

bool
WickrIOSSLSettings::validateSSLKey(const QString &fileName)
{
    if (! fileName.isEmpty()) {
        QFile sslFile(fileName);
        QFileInfo info(sslFile);

        if (sslFile.exists() && info.isFile()  && info.size() > 0) {
            return true;
        }
    }
    return false;
}

bool
WickrIOSSLSettings::validateSSLCert(const QString &fileName)
{
    if (! fileName.isEmpty()) {
        QFile sslFile(fileName);
        QFileInfo info(sslFile);

        if (sslFile.exists() && info.isFile() && info.size() > 0) {
            return true;
        }
    }
    return false;
}


bool
WickrIOAppSettings::setupEmail(QString server, int port, QString type, QString account, QString password, QString sender, QString recipient, QString subject)
{
    // Validate the input information as much as possible
    if (server.isEmpty() || port <= 0 || type.isEmpty() || account.isEmpty() || password.isEmpty() ||
        sender.isEmpty() || recipient.isEmpty() || subject.isEmpty()) {
        return false;
    }
    if (type.toLower() != "smtp" && type.toLower() != "ssl" && type.toLower() != "tls") {
        return false;
    }

    // Convert the input into a json object and save it
    QJsonObject emailSetup;

    QJsonObject serverSetup;
    serverSetup.insert("server", server);
    serverSetup.insert("port", port);
    serverSetup.insert("type", type);
    serverSetup.insert("account", account);
    serverSetup.insert("password", password);
    emailSetup.insert("server_setup", serverSetup);

    QJsonObject msgSetup;
    msgSetup.insert("sender", sender);
    msgSetup.insert("recipient", recipient);
    msgSetup.insert("subject", subject);
    emailSetup.insert("message_setup", msgSetup);

    QJsonDocument saveDoc(emailSetup);
    QByteArray byteArray = saveDoc.toJson();

    value = QString(byteArray);
    return true;
}

bool
WickrIOAppSettings::getEmail(WickrIOEmailSettings *email)
{
    if (!value.isEmpty()) {
        QJsonParseError jsonError;
        QByteArray jsonString;
        jsonString.append(value);
        QJsonDocument jsonDoc = QJsonDocument().fromJson(jsonString, &jsonError);

        if (jsonError.error != jsonError.NoError) {
            return false;
        }

        QJsonObject emailSetup = jsonDoc.object();
        QJsonValue value;

        if (emailSetup.contains("server_setup") &&
            emailSetup.contains("message_setup")) {
            value = emailSetup["server_setup"];
            QJsonObject serverSetup = value.toObject();
            if (serverSetup.contains("server") &&
                serverSetup.contains("port") &&
                serverSetup.contains("type") &&
                serverSetup.contains("account") &&
                serverSetup.contains("password")) {
                value = serverSetup["server"];
                email->server = value.toString();
                value = serverSetup["port"];
                email->port = value.toInt();
                value = serverSetup["type"];
                email->type = value.toString();
                value = serverSetup["account"];
                email->account = value.toString();
                value = serverSetup["password"];
                email->password = value.toString();
            } else {
                return false;
            }

            value = emailSetup["message_setup"];
            QJsonObject msgSetup = value.toObject();
            if (msgSetup.contains("sender") &&
                msgSetup.contains("recipient") &&
                msgSetup.contains("subject")) {
                value = msgSetup["sender"];
                email->sender = value.toString();
                value = msgSetup["recipient"];
                email->recipient = value.toString();
                value = msgSetup["subject"];
                email->subject = value.toString();
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}
