#ifndef WICKRIODATABASE_H
#define WICKRIODATABASE_H

#include <QString>
#include <QList>

#include "wickrbotclientdatabase.h"

#include "wickrIOAppSettings.h"
#include "wickrioconsoleuser.h"
#include "wickriotokens.h"
#include "wickriomessage.h"
#include "wickrIOParsers.h"

/**
 * @brief The WickrIODBAttachment class
 */
class WickrIODBAttachment {
public:
    QString m_filename;
    QString m_realfilename;

    WickrIODBAttachment(QString filename, QString realfilename) :
        m_filename(filename),
        m_realfilename(realfilename) {}
};

/**
 * @brief The WickrIODBUser class
 * This class is associated with the user record
 */
class WickrIODBUser {
public:
    int         m_id;               // Unique ID value
    QString     m_user;             // Wickr User ID for this user
    int         m_permission;       // Permissions (admin)
    int         m_clientCount;      // Max clients allowed to control
    int         m_motherBotID;      // ID of associated mother bot
    QList<int>  m_clients;          // IDs of the associated clients

    WickrIODBUser() :
        m_id(-1),
        m_permission(0),
        m_clientCount(0),
        m_motherBotID(-1)
    {}

    WickrIODBUser(int id, const QString& user, int permission, int maxClents, int motherBotID) :
        m_id(id),
        m_user(user),
        m_permission(permission),
        m_clientCount(maxClents),
        m_motherBotID(motherBotID) {}

    bool isAdmin() { return(m_permission & CUSER_PERM_ADMIN_FLAG); }
    void setAdmin(bool admin);
    bool canEdit() { return m_permission & CUSER_PERM_EDIT_FLAG; }
    void setEdit(bool edit);
    bool canCreate() { return m_permission & CUSER_PERM_CREATE_FLAG; }
    void setCreate(bool create);
    bool rxEvents() { return m_permission & CUSER_PERM_EVENTS_FLAG; }
    void setRxEvents(bool rxEvents);

};

/**
 * @brief The WickrIOClientDatabase class
 */
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

    WickrBotClients *getClientUsingName(QString name);
    int getClientsConsoleID(int id);
    QList<WickrBotClients *> getClients();
    QList<WickrBotClients *> getMotherBotClients(bool getMBots);
    QList<WickrBotClients *> getConsoleClients(int console_id);
    int numConsoleClients(int console_id);
    QString getClientsConsoleUser(int id);
    bool deleteClientsOfConsoleUser(int cUserID);
    bool clearClientsOfConsoleUser(int cUserID);
    bool insertClientsRecord(WickrBotClients *client);
    bool updateClientsRecord(WickrBotClients *client, bool insertIfNotExist=true);

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
    bool deleteMessage(int id, bool saveAttachment);
    bool messageHasAttachments(int id);
    int getClientsOutMessagesCount(int clientID);

    bool insertAttachment(int messageID, const QString &filename, const QString &realFilename);
    QList<WickrIODBAttachment *> getAttachments(int messageID);

    int insertUser(const QString& user, int permissions, int maxClients, int motherID);
    int insertUser(WickrIODBUser *user);
    bool deleteUser(int id);
    bool updateUser(WickrIODBUser *user);
    QList<WickrIODBUser *> getUsers();
    QList<WickrIODBUser *> getMotherUsers(int motherID);
    bool getUser(const QString& name, int motherID, WickrIODBUser *user);

    int insertUserClient(int userID, int clientID);
    bool deleteUserClient(int id);
    bool deleteUsersClients(int user_id);
    QList<int> getUserClients(int user_id);

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

#define DB_USERS_TABLE                      "users"
#define DB_USERCLIENTS_TABLE                "user_clients"

/*
 * Definitions for the Statistics table, which is defined in WickrBotDatabase
 */
#define DB_STATID_MSGS_TX           1
#define DB_STATID_MSGS_RX           2
#define DB_STATID_ERRORS_TX         3
#define DB_STATID_ERRORS_RX         4
#define DB_STATID_MSGS_OBOXSYNC     5

#define DB_STATDESC_TOTAL           "total"




#endif // WICKRIODATABASE_H
