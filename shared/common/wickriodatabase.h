#ifndef WICKRIODATABASE_H
#define WICKRIODATABASE_H

#include <QString>
#include <QList>

#include "wickrbotclientdatabase.h"

#include "wickrIOAppSettings.h"
#include "wickrioconsoleuser.h"
#include "wickrioclients.h"
#include "wickriotokens.h"
#include "wickriomessage.h"
#include "wickrIOParsers.h"

class WickrIODBAttachment {
public:
    QString m_filename;
    QString m_realfilename;

    WickrIODBAttachment(QString filename, QString realfilename) :
        m_filename(filename),
        m_realfilename(realfilename) {}
};

class WickrIOClientDatabase : public WickrBotClientDatabase
{
    Q_OBJECT

public:
    WickrIOClientDatabase(const QString &dirPath);

    bool insertAppSetting(int clientID, const QString &type, const QString &url);
    bool updateAppSetting(WickrIOAppSettings *appSetting);
    bool getAppSetting(int id, WickrIOAppSettings *appSettings);
    bool getAppSetting(int clientID, QString type, WickrIOAppSettings *appSettings);
    bool deleteAppSetting(int id);

    bool insertConsoleUser(WickrIOConsoleUser *cuser);
    bool insertConsoleUser(const QString &name, const QString &password, int permissions, int max_clients, int authType, QString email);
    bool updateConsoleUser(WickrIOConsoleUser *cUser);
    QList<WickrIOConsoleUser *> getConsoleUsers();
    int numConsoleUsers();
    bool getConsoleUser(int id, WickrIOConsoleUser *cUser);
    bool getConsoleUser(const QString &user, WickrIOConsoleUser *cUser);
    bool deleteConsoleUser(int id);
    bool deleteConsoleUser(const QString &user);

    WickrIOClients *getClientUsingName(QString name);
    int getClientsConsoleID(int id);
    QList<WickrIOClients *> getClients();
    QList<WickrIOClients *> getConsoleClients(int console_id);
    int numConsoleClients(int console_id);
    QString getClientsConsoleUser(int id);
    bool deleteClientsOfConsoleUser(int cUserID);
    bool clearClientsOfConsoleUser(int cUserID);
    bool insertClientsRecord(WickrIOClients *client);
    bool updateClientsRecord(WickrIOClients *client, bool insertIfNotExist=true);

    QList<WickrIOParsers *> getParsers();

    bool insertToken(WickrIOTokens *token);
    bool insertToken(const QString &token, int console_id, const QString &remote);
    bool updateToken(WickrIOTokens *token);
    QList<WickrIOTokens *> getTokens();
    bool getToken(int id, WickrIOTokens *token);
    bool getToken(const QString &tokenStr, WickrIOTokens *token);
    bool deleteToken(int id);
    bool deleteToken(const QString &token);

    bool getTokenConsoleUser(const QString &token, WickrIOConsoleUser *cUser);
    bool getConsoleUserToken(int id, WickrIOTokens *token);

    int insertMessage(long timestamp, int clientID, const QString &json, int type, int hasAttachment=0);
    bool updateMessage(WickrIOMessage *message);
    bool getMessage(int id, WickrIOMessage *message);
    QList<int> getMessageIDs(int clientID);
    bool deleteMessage(int id);
    bool messageHasAttachments(int id);
    int getClientsOutMessagesCount(int clientID);

    bool insertAttachment(int messageID, const QString &filename, const QString &realFilename);
    QList<WickrIODBAttachment *> getAttachments(int messageID);

protected:
    bool createRelationalTables();

private:

};

#define DB_APPSETTINGS_TABLE                "app_settings"
#define DB_APPSETTINGS_TYPE_MSGRECVCALLBACK "MsgRecvCallback"
#define DB_APPSETTINGS_TYPE_MSGRECVEMAIL    "MsgRecvEmail"

#define DB_CONSOLEUSERS_TABLE               "console_users"

#define DB_TOKEN_TABLE                      "tokens"

#define DB_MESSAGES_TABLE                   "messages"
#define DB_ATTACHMENTS_TABLE                "attachments"

/*
 * Definitions for the Statistics table, which is defined in WickrBotDatabase
 */
#define DB_STATID_MSGS_TX           1
#define DB_STATID_MSGS_RX           2
#define DB_STATID_ERRORS_TX         3
#define DB_STATID_ERRORS_RX         4

#define DB_STATDESC_TOTAL           "total"

#endif // WICKRIODATABASE_H
