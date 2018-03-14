#ifndef WICKRBOTDATABASE_H
#define WICKRBOTDATABASE_H

#include <QSqlRelationalTableModel>
#include <QString>

#include "wickrbotlib.h"
#include "wickrbotactioncache.h"
#include "wickrbotprocessstate.h"
#include "wickrbotclients.h"
#include "wickrbotclientevents.h"
#include "wickrbotstatistics.h"

class DECLSPEC WickrBotDatabase : public QObject
{
    Q_OBJECT

public:
    WickrBotDatabase(const QString &dirPath);
    virtual ~WickrBotDatabase();

    void close();
    bool isOpen() { return m_db.isOpen(); }

    int getNextID(const QString &table);
    bool touchRecord(int id, const QString &table);

    bool insertAttachment(const QString &url, const QString &filename, int filesize);
    QString getAttachment(const QString &url);

    bool insertAction(const QString &json, QDateTime runtime, int clientID);
    bool getAction(int id, WickrBotActionCache *action);
    bool deleteAction(int id);
    void dumpActions();
    int getClientsActionCount(int clientID);

    bool getProcessState(const QString &process, WickrBotProcessState *state);
    bool insertProcessState(const QString &process, int process_id, int state);
    bool updateProcessState(const QString &process, int process_id, int state);
    bool deleteProcessState(const QString& process);
    bool setProcessIPC(const QString &process, int ipc_port);

    WickrBotClients *getClient(int id);
    WickrBotClients *getClientUsingApiKey(QString apiKey);
    WickrBotClients *getClientUsingName(QString userName);
    WickrBotClients *getClientUsingUserName(QString userName);
    QList<WickrBotClients *> getClients();
    bool insertClientsRecord(WickrBotClients *client);
    bool updateClientsRecord(WickrBotClients *client, bool insertIfNotExist=true);
    QSqlQueryModel *getClientsModel();
    bool deleteClientUsingName(QString name);
    void getClient(QSqlQuery *query, WickrBotClients *client);
    QList<int> getClientIDFromType(const QString& clientType);
    int numberOfActionsForClient(int clientid);

    QList<WickrBotProcessState *> getProcessStates();
    void getProcess(QSqlQuery *query, WickrBotProcessState *processState);

    bool getClientEvent(int id, WickrBotClientEvents *event);
    void getClientEventFromQuery(QSqlQuery *query, WickrBotClientEvents *event);
    QList<WickrBotClientEvents *> getClientEvents(int maxCount);
    bool insertClientEventRecord(int clientID, const QString& message, WickrBotClientEvents::WickrIOEventType type);
    bool deleteClientEvent(int id);
    bool getFirstClientEvent(QString dateTime, WickrBotClientEvents *event);

    virtual bool updateLastUserMessage(const QString &) {return false;}
    virtual QDateTime getLastUserMessageTime(const QString &) {return QDateTime::currentDateTime();}
    virtual bool deleteLastUserMessage(const QString &) {return false;}
    virtual bool deleteLastUserOlderThan(const long) {return false;}

    void getStatistic(QSqlQuery *query, WickrBotStatistics *stat);
    QList<WickrBotStatistics *> getStatistics();
    QList<WickrBotStatistics *> getClientStatistics(int clientID);
    bool deleteClientStatistics(int clientID);
    bool insertStatistic(int clientID, int statID, const QString &statDesc, qlonglong statValue);
    bool incStatistic(int clientID, int statID, const QString &statDesc, qlonglong increment);

    bool getFirstAction(QString dateTime, WickrBotActionCache *);
    bool getFirstAction(QString dateTime, WickrBotActionCache *, int);

    virtual QString dumpTableCounts();

    int size();

    static bool getFirstAction(WickrBotDatabase *botDB, const QString& dateTime, WickrBotActionCache *action);
    static bool getFirstAction(WickrBotDatabase *botDB, const QString& dateTime, WickrBotActionCache *action, int clientID);

    QSqlDatabase m_db;
    QString m_dbDir;

protected:
    bool initialized;
    QString m_sDatabaseFileName;

protected:
    void initializeModel(QSqlRelationalTableModel *);
    bool createConnection();
    virtual bool createRelationalTables();
    QString getClientTime();
    QString getClientTimeWithOffset(long seconds);

    bool runSimpleQuery(QString qryString);

};

#define DB_DATETIME_FORMAT      "yyyy-MM-dd hh:mm:ss"

#define DB_ATTACHMENT_TABLE     "attachment_cache"
#define DB_ACTION_TABLE         "action_cache"

#define DB_STATE_TABLE          "process_state"
#define DB_STATISTICS_TABLE     "statistics"

// The Clients table identifies all of the possible clients running on the
// WickrBot server. Also details of each of the clients.
#define DB_CLIENTS_TABLE        "clients"

// The Client Events table contains list of events from the clients
#define DB_CLIENTEVENTS_TABLE   "client_events"

#endif // WICKRBOTDATABASE_H
