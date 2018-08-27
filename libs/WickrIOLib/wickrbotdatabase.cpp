#include <QtSql>
#include <QDebug>
#include <QDateTime>
#include "wickrbotdatabase.h"
#include "wickrbotactioncache.h"
#include "wickrbotprocessstate.h"
#include "wickrbotclients.h"

namespace WickrCore {
    class WickrSession;
};

/**
 * @brief WickrBotDatabase::WickrBotDatabase
 * Constructor for the WickrBotDatabase class. This constructor will call internal methods to setup
 * and initialize the WickrBot database.
 */
WickrBotDatabase::WickrBotDatabase(const QString &dirPath) :
    m_dbDir(dirPath)
{
    m_sDatabaseFileName = dirPath + "/wickrbot.database.sqlite";
    if (!createConnection()) {
        initialized = false;
    } else {
        createRelationalTables();
        initialized = true;
    }
}

WickrBotDatabase::~WickrBotDatabase()
{
    if (initialized) {
        initialized = false;
        if (m_db.isOpen())
            m_db.close();
    }
}

/**
 * @brief WickrBotDatabase::createRelationalTables
 * This method will create the tables for the WickrBot database.
 * @return True is returned if created successfully and false if not
 */
bool
WickrBotDatabase::createRelationalTables()
{
    // Check if the attachment cache table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_ATTACHMENT_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("create table attachment_cache (id int primary key, url text unique, filename text unique, filesize int, created timestamp, lastaccess timestamp)")) {
            qDebug() << "create table failed, query error=" << query.lastError();
            query.finish();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the clients table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_CLIENTS_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE clients (id int primary key, name text NOT NULL UNIQUE, port int, interface text, api_key text NOT NULL, user text NOT NULL, isHttps int, sslKeyFile text, sslCertFile text, binary text NOT NULL)")) {
            qDebug() << "create clients table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the client events table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_CLIENTEVENTS_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE client_events (id int primary key, client_id int, message text NOT NULL UNIQUE, event_time timestamp, type int)")) {
            qDebug() << "create client eventss table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the action cache table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_ACTION_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE action_cache (id int primary key, json text, created timestamp, runtime timestamp, attempts int, client_id int, FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE)")) {
            qDebug() << "create table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the process state table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_STATE_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE process_state (id int primary key, process text NOT NULL UNIQUE, process_id int, state int, last_update timestamp)")) {
            qDebug() << "create process_state table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the statistics table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_STATISTICS_TABLE))) {
        QSqlQuery query(m_db);
        query.exec("PRAGMA foreign_keys = ON;");
        // TODO: Need to add some indices to make queries faster!
        if (!query.exec("CREATE TABLE statistics (id int primary key, client_id int NOT NULL, stat_id int NOT NULL, stat_desc text, stat_value long, FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE)")) {
            qDebug() << "create statistics table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
        if (!runSimpleQuery("CREATE INDEX stats_index on statistics(client_id, stat_id, stat_desc)")) {
            qDebug() << "create statistics index failed, query error=" << query.lastError();
        }
    }

    return true;
}


/**
 * @brief WickrBotDatabase::createConnection
 * This method will open the WickrBot database.
 * @return Returns true if the database is open, false if not
 */
bool
WickrBotDatabase::createConnection()
{
    m_db = QSqlDatabase::addDatabase("QSQLCIPHER_WICKR", "WickrBotDBConnection");
    qDebug() << QSqlDatabase::drivers();

    m_db.setDatabaseName(m_sDatabaseFileName);
    if (!m_db.open()) {
        qDebug() << qApp->tr("Cannot open database") << m_db.lastError();
        qDebug() << qApp->tr("Unable to establish a database connection.\n"
                     "This example needs SQLite support. Please read "
                     "the Qt SQL driver documentation for information how "
                     "to build it.\n\n"
                     "Click Cancel to exit.");
        return false;
    }

    return true;
}

/**
 * @brief WickrBotDatabase::close
 * Close the database if it is open. Set teh initialized flag to false so that no
 * other commands will be attempted.
 */
void
WickrBotDatabase::close()
{
    if (initialized) {
        initialized = false;
        if (m_db.isOpen()) {
            QString conn = m_db.connectionName();
            m_db.close();
            m_db.removeDatabase(conn);
        }
    }
}

/**
 * @brief WickrBotDatabase::alterTable
 * Perform the input alter table query
 * @param qryString
 * @return
 */
bool
WickrBotDatabase::runSimpleQuery(QString qryString)
{
    bool success=false;
    QSqlQuery alterQuery(m_db);

    alterQuery.prepare(qryString);
    if ( !alterQuery.exec()) {
        QSqlError err = alterQuery.lastError();
        qDebug() << "Could not perform simple SQL Query, Error=" << err;
    } else {
        success = true;
    }
    alterQuery.finish();
    return success;
}

/****************************************************************************************************************
 * Attachment Cache (attachment_cache) handler functions
 ***************************************************************************************************************/

/**
 * @brief WickrBotDatabase::insertAttachment
 * This method will insert a new record into the attachment_cache table. The created and lastaccess
 * datetime value will be set to the current time. The ID will be generated and set.
 * @param url The URL of the new record
 * @param filename The filename of the new record
 * @param filesize The size of the file associated with the new record
 * @return true if successful, false if not
 */
bool
WickrBotDatabase::insertAttachment(const QString &url, const QString &filename, int filesize) {
    if (!initialized)
        return false;
    bool retval = false;

    QString dateTime = getClientTime();

    int id = getNextID(DB_ATTACHMENT_TABLE);

    // TODO: Need to parse out the filename and url to make sure there aren't any special characters

#if 1
    QString queryString = "INSERT INTO attachment_cache (id, url, filename, filesize, created, lastaccess) VALUES (?, '?', '?', ?, '?', '?')";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);
    query.bindValue(1, url);
    query.bindValue(2, filename);
    query.bindValue(3, filesize);
    query.bindValue(4, dateTime);
    query.bindValue(5, dateTime);
#else
    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO attachment_cache (id, url, filename, filesize, created, lastaccess) VALUES (%1, '%2', '%3', %4, '%5', '%6')")
            .arg(id)
            .arg(url)
            .arg(filename)
            .arg(filesize)
            .arg(dateTime)
            .arg(dateTime);
#endif
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insert Attachment: SQL error" << error;
        qDebug() << "QUERY=" << queryString;
        qDebug() << "FILENAME=" << filename;
        qDebug() << "URL=" << url;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

/**
 * @brief WickrBotDatabase::getAttachment
 * This method will retrieve an attachment filename that is associated with the input URL.
 * @param url The URL of the attachment record to retrieve.
 * @return QString that is the filename of retrieved record, NULL if not found
 */
QString
WickrBotDatabase::getAttachment(const QString &url) {
    if (!initialized)
        return NULL;

    QString retstring("");

    QString queryString = "SELECT id,filename,created,lastaccess FROM attachment_cache WHERE url = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, url);

    if ( !query.exec()) {
        qDebug() << "Could not retrieve" << url;
    } else if (query.next()) {
        int id = query.value(0).toInt();
        QString filename = query.value(1).toString();
        QDateTime dateTime = query.value(2).toDateTime();
        QDateTime lastAccess = query.value(3).toDateTime();

        qDebug() << "Attachement id=" << id << "has createDateTime of" << dateTime.toString(DB_DATETIME_FORMAT) << ", lastaccess of" << lastAccess.toString(DB_DATETIME_FORMAT);
        // Need to update the accessed time
        touchRecord(id, DB_ATTACHMENT_TABLE);

        retstring = filename;
    }
    query.finish();
    return retstring;
}

/****************************************************************************************************************
 * Action Cache (action_cache) handler functions
 ***************************************************************************************************************/

/**
 * @brief WickrBotDatabase::insertAction
 * This method will insert a new action into the cache. The id value will be generated. The created
 * time will be the current time and the number of attempts will be set to 0.
 * @param json The JSON string to put in the record
 * @param runtime The date/time when the command should be performed.
 * @return true is returned if the action was inserted successfully, false if not
 */
bool
WickrBotDatabase::insertAction(const QString &json, QDateTime runtime, int clientID) {
    if (!initialized)
        return false;

    // Creation times are based on the client's time and date
    QString createTime = getClientTime();

    int id = getNextID(DB_ACTION_TABLE);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO action_cache (id, json, created, runtime, attempts, client_id) "
                  "VALUES (:id, :json, :created, :runtime, :attempts, :client_id)");
    query.bindValue(":id", id);
    query.bindValue(":json", json);
    query.bindValue(":created", createTime);
    query.bindValue(":runtime", runtime.toString(DB_DATETIME_FORMAT));
    query.bindValue(":attempts", 0);
    query.bindValue(":client_id", clientID);
    if (!query.exec()) {
        QSqlError error = query.lastError();
        qDebug() << "insertAction: SQL error" << error;
        query.finish();
        return false;
    }
    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/**
 * @brief WickrBotDatabase::getAction
 * This method will retrieve a WickrBotActionCache record that has the input ID value.
 * @param id The ID of the record to retrieve.
 * @param action Pointer to the action record to populate
 * @return true if the record is found, false if not
 */
bool
WickrBotDatabase::getAction(int id, WickrBotActionCache *action) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT json,created,runtime,attempts FROM action_cache WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getAction: Could not retrieve" << id;
    } else if (query.next()) {
        action->id = id;
        action->json = query.value(0).toByteArray();
        action->created = query.value(1).toDateTime();
        action->runTime = query.value(2).toDateTime();
        action->attempts = query.value(3).toInt();
        retval = true;
    }
    query.finish();
    return retval;
}

/**
 * @brief WickrBotDatabase::deleteAction
 * This method will delete the action record with the input ID.
 * @param id The id of the action to delete
 * @return true if a record is deleted, false if nothing deleted
 */
bool
WickrBotDatabase::deleteAction(int id) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "DELETE FROM action_cache WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteAction: Could not retrieve" << id << "last error=" << query.lastError();
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

/**
 * @brief WickrBotDatabase::dumpActions
 * This function will dump to the debug output all of the current records in the action_cache
 * table.
 */
void
WickrBotDatabase::dumpActions() {
    if (!initialized)
        return;

    QString queryString = "SELECT id,json,created,runtime,attempts FROM action_cache";
    QSqlQuery query(m_db);
    query.prepare(queryString);

    if ( !query.exec()) {
        qDebug() << "dumpActions: Could not retrieve";
    } else {

        while (query.next()) {
            int id = query.value(0).toInt();
            QString json = query.value(1).toString();
            QDateTime created = query.value(2).toDateTime();
            QDateTime runTime = query.value(3).toDateTime();
            int attempts = query.value(4).toInt();

            QString curEntry = QString("Action (%1): runtime=%2, created=%3, attempts=%4, JSON=%5")
                    .arg(id)
                    .arg(runTime.toString(DB_DATETIME_FORMAT))
                    .arg(created.toString(DB_DATETIME_FORMAT))
                    .arg(attempts)
                    .arg(json);
            qDebug() << curEntry;
        }
    }
    query.finish();
}

bool
WickrBotDatabase::getFirstAction(QString dateTime, WickrBotActionCache *action) {
    if (!initialized)
        return false;

    QSqlQuery query(m_db);
    QString queryString = QString("SELECT MIN(id) FROM action_cache WHERE runtime <= '%1'")
            .arg(dateTime);
    if ( !query.exec(queryString)) {
        qDebug() << "getFirstAction: Could not retrieve action. DBError=" << query.lastError();
        query.finish();
        return false;
    }

    if (query.next()) {
        int id = query.value(0).toInt();
        query.finish();
        return getAction(id, action);
    } else {
        qDebug() << "getFirstAction: No rows retrieved";
        query.finish();
        return false;
    }
}

bool
WickrBotDatabase::getFirstAction(QString dateTime, WickrBotActionCache *action, int clientID) {
    if (!initialized)
        return false;

    QSqlQuery query(m_db);
    QString queryString = QString("SELECT MIN(id) FROM action_cache WHERE runtime <= '%1' and client_id = %2")
            .arg(dateTime)
            .arg(clientID);
    if ( !query.exec(queryString)) {
        qDebug() << "getFirstAction: Could not retrieve action. DBError=" << query.lastError();
        query.finish();
        return false;
    }

    if (query.next()) {
        int id = query.value(0).toInt();
        query.finish();
        return getAction(id, action);
    } else {
        query.finish();
        qDebug() << "getFirstAction: No rows retrieved";
        return false;
    }
}

/**
 * @brief WickrBotDatabase::getClientsActionCount
 * Get the number of action_cache records associated with the input client ID
 * @param clientID The id of the client
 * @return 0 if none found or if an error occured
 */
int
WickrBotDatabase::getClientsActionCount(int clientID)
{
    int retVal = 0;

    if (!initialized)
        return retVal;

    QString queryString = "SELECT count(id) FROM action_cache WHERE client_id=?";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);
    query->bindValue(0, clientID);

    if ( query->exec()) {
        if (query->next()) {
            retVal = query->value(0).toInt();
        }
    }
    query->finish();
    delete query;
    return retVal;
}


/****************************************************************************************************************
 * Process State handler functions
 ***************************************************************************************************************/

/**
 * @brief WickrBotDatabase::getProcessState
 * This function will retrieve the WickrBotProcessState record associated with the input Process.
 * @param process The name of the process to retrieve.
 * @param state Pointer to the WickrBotProcessState object to put the retrieved information
 * @return true is returned if the record is found, false if not or the query failes
 */
bool
WickrBotDatabase::getProcessState(const QString &process, WickrBotProcessState *state) {
    if (!initialized)
        return false;

    bool retval = false;
    int retryCnt = 5;

    QString queryString = "SELECT id,process_id,state,last_update FROM process_state WHERE process = ?";
    QSqlQuery *query;
    bool connectionError;

    // Do the query for the process state. Repeat this if there is a connection error.
    do {
        query = new QSqlQuery(m_db);
        query->prepare(queryString);
        query->bindValue(0, process);
        if ( query->exec()) {
            break;
        }

        qDebug() << "getProcessState: Could not retrieve" << process << ", Error=" << query->lastError();
        if (query->lastError().type() != QSqlError::ConnectionError || !isOpen()) {
            connectionError = false;
        } else {
            connectionError = true;
        }

        // Cleanup the query
        query->finish();
        delete query;
        query = NULL;

        if (connectionError) {
            QThread::msleep(100);
        } else {
            break;
        }

    } while (--retryCnt > 0);

    if (query != NULL) {
        if (query->next()) {
            state->id = query->value(0).toInt();
            state->process_id = query->value(1).toInt();
            state->state = query->value(2).toInt();
            state->last_update = query->value(3).toDateTime();
            state->process = process;
            retval = true;
        }
        query->finish();
        delete query;
    }
    return retval;
}

/**
 * @brief WickrBotDatabase::insertProcessState
 * This function will insert a Process State entry into the process_state table in the database.
 * @param process The name of the process
 * @param process_id The process ID associated with the active process
 * @param state The current state of the process
 * @return true is returned if the database operates were successful
 */
bool
WickrBotDatabase::insertProcessState(const QString &process, int process_id, int state) {
    if (!initialized)
        return false;

    // The ProcessState will use local time since this has to operate independent of the server
    QString dateTime = getClientTime();
    qDebug() << "insertProcessState: Date is " << dateTime;

    int id = getNextID(DB_STATE_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO process_state (id, process, process_id, state, last_update) VALUES (%1, '%2', %3, %4, '%5')")
            .arg(id)
            .arg(process)
            .arg(process_id)
            .arg(state)
            .arg(dateTime);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertProcessState: SQL error" << error;
        query.finish();
        return false;
    }
    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/**
 * @brief WickrBotDatabase::updateProcessState
 * This function will update the Process state value in the database. If there is no current value
 * in the database then one will be inserted. If there is an existing value associated with the
 * input Process value then that entry will be updated with the input process ID and state values.
 * @param process String value that uniquely identifies the process.
 * @param process_id The process ID of the process.
 * @param state The state of the process. Values are found in the header file
 * @return  true is returned if the operation was successful, false otherwise
 */
bool
WickrBotDatabase::updateProcessState(const QString &process, int process_id, int state) {
    if (!initialized)
        return false;

    WickrBotProcessState processState;

    // If the process state value does not exist then insert a new one
    if (! getProcessState(process, &processState)) {
        return insertProcessState(process, process_id, state);
    }

    // The ProcessState will use local time since this has to operate independent of the server
    QString dateTime = getClientTime();
    qDebug() << QString("updateProcessState[%1]: Date is %2, state=%3").arg(process).arg(dateTime).arg(state);

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE process_state SET process_id=%1, state=%2, last_update='%3' WHERE process='%4'")
            .arg(process_id)
            .arg(state)
            .arg(dateTime)
            .arg(process);

    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateProcessState: SQL error" << error;
        query.finish();
        return false;
    }
    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/**
 * @brief WickrBotDatabase::deleteProcessState
 * This function will delete the process_state record with the input process name.
 * @param process
 * @return
 */
bool
WickrBotDatabase::deleteProcessState(const QString& process) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM process_state WHERE process = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, process);

    if ( !query.exec()) {
        qDebug() << "deleteProcessState: Could not delete record for" << process;
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}



/****************************************************************************************************************
 * Clients (clients) handler functions
 *
 * The clients table has the following columns:
 * id int primary key
 * name text NOT NULL UNIQUE
 * port int
 * interface text
 * api_key text NOT NULL UNIQUE
 * user text NOT NULL UNIQUE
 * isHttps int
 * sslKeyFile text
 * sslCertFile text
 ***************************************************************************************************************/

WickrBotClients *WickrBotDatabase::getClient(int id)
{
    WickrBotClients * client = NULL;

    if (!initialized)
        return client;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary FROM clients WHERE id=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getClient: Could not retrieve, Error=" << query.lastError();
    } else {
        if (query.next()) {
            client = new WickrBotClients();

            client->id = query.value(0).toInt();
            client->name = query.value(1).toString();
            client->port = query.value(2).toInt(0);
            client->iface = query.value(3).toString();
            client->apiKey = query.value(4).toString();
            client->user = query.value(5).toString();
            int isHttps = query.value(6).toInt();
            client->isHttps = (isHttps == 1) ? true : false;
            client->sslKeyFile = query.value(7).toString();
            client->sslCertFile = query.value(8).toString();
            client->binary = query.value(9).toString();
        }
    }
    query.finish();
    return client;
}

WickrBotClients *
WickrBotDatabase::getClientUsingApiKey(QString apiKey)
{
    WickrBotClients * client = NULL;

    if (!initialized)
        return client;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary FROM clients WHERE api_key=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, apiKey);

    if ( !query.exec()) {
        qDebug() << "getClientUsingApiKey: Could not retrieve, Error=" << query.lastError();
    } else {
        if (query.next()) {
            client = new WickrBotClients();

            client->id = query.value(0).toInt();
            client->name = query.value(1).toString();
            client->port = query.value(2).toInt(0);
            client->iface = query.value(3).toString();
            client->apiKey = query.value(4).toString();
            client->user = query.value(5).toString();
            int isHttps = query.value(6).toInt();
            client->isHttps = (isHttps == 1) ? true : false;
            client->sslKeyFile = query.value(7).toString();
            client->sslCertFile = query.value(8).toString();
            client->binary = query.value(9).toString();
        }
    }
    query.finish();
    return client;
}

WickrBotClients *
WickrBotDatabase::getClientUsingName(QString name)
{
    WickrBotClients * client = NULL;

    if (!initialized)
        return client;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary FROM clients WHERE name=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, name);

    if ( !query.exec()) {
        qDebug() << "getClientUsingName: Could not retrieve, Error=" << query.lastError();
    } else {
        if (query.next()) {
            client = new WickrBotClients();
            getClient(&query, client);
        }
    }
    query.finish();
    return client;
}

WickrBotClients *
WickrBotDatabase::getClientUsingUserName(QString userName)
{
    WickrBotClients * client = NULL;

    if (!initialized)
        return client;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary FROM clients WHERE user=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, userName);

    if ( !query.exec()) {
        qDebug() << "getClientUsingUserName: Could not retrieve, Error=" << query.lastError();
    } else {
        if (query.next()) {
            client = new WickrBotClients();
            getClient(&query, client);
        }
    }
    query.finish();
    return client;
}

/**
 * @brief WickrBotDatabase::getClient
 * This function will set the values known at this level of the WickrBot clients
 * class type. This class has been derived so this function can be used to bet
 * the base class values.
 * @param query Pointer to the current query that is at a position for a record
 * @param client Pointer to the base or derived class
 * @return
 */
void
WickrBotDatabase::getClient(QSqlQuery *query, WickrBotClients *client)
{
    QSqlRecord rec = query->record();

    client->id = query->value(rec.indexOf("id")).toInt();
    client->name = query->value(rec.indexOf("name")).toString();
    client->port = query->value(rec.indexOf("port")).toInt(0);
    client->iface = query->value(rec.indexOf("interface")).toString();
    client->apiKey = query->value(rec.indexOf("api_key")).toString();
    client->user = query->value(rec.indexOf("user")).toString();
    int isHttps = query->value(rec.indexOf("isHttps")).toInt();
    client->isHttps = (isHttps == 1) ? true : false;
    client->sslKeyFile = query->value(rec.indexOf("sslKeyFile")).toString();
    client->sslCertFile = query->value(rec.indexOf("sslCertFile")).toString();
    client->binary = query->value(rec.indexOf("binary")).toString();
    client->botType = query->value(rec.indexOf("integration_type")).toString();
}

int
WickrBotDatabase::numberOfActionsForClient(int clientid)
{
    int count = 0;
    QString query = QString("SELECT COUNT(*) from action_cache where client_id=%1").arg(clientid);

    QSqlQuery   doQuery;
    doQuery.prepare(query);
    doQuery.exec();
    if ( doQuery.isActive() ) {
        if (doQuery.next()) {
            count = doQuery.value(0).toInt();
        }
    }
    doQuery.finish();
    return count;
}

QList<int>
WickrBotDatabase::getClientIDFromType(const QString& clientType)
{
    QList<int> clientIDs;
    if (!initialized)
        return clientIDs;

    QString queryString = QString("SELECT id FROM clients where binary='%1'").arg(clientType);
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);

    if ( !query->exec()) {
        qDebug() << "getClients: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            int id = query->value(0).toInt();
            clientIDs.append(id);
        }
    }
    query->finish();
    delete query;
    return clientIDs;
}

QList<WickrBotClients *>
WickrBotDatabase::getClients()
{
    QList<WickrBotClients *> clients;
    if (!initialized)
        return clients;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary,integration_type FROM clients";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);

    if ( !query->exec()) {
        qDebug() << "getClients: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            WickrBotClients *client = new WickrBotClients();
            getClient(query, client);
            clients.append(client);
        }
    }
    query->finish();
    delete query;
    return clients;
}


QList<WickrBotProcessState *>
WickrBotDatabase::getProcessStates()
{
  QList<WickrBotProcessState *> processes;
  if (!initialized)
    return processes;

  QString queryString = "SELECT id,process,process_id, state, last_update FROM process_state";
  QSqlQuery *query = new QSqlQuery(m_db);
  query->prepare(queryString);

  if ( !query->exec()) {
    qDebug() << "getProcessStates: Could not retrieve, Error=" << query->lastError();
  } else {
    while (query->next()) {
      WickrBotProcessState *process = new WickrBotProcessState();
      getProcess(query, process);
      processes.append(process);
    }
  }
  query->finish();
  delete query;
  return processes;
}

void
WickrBotDatabase::getProcess(QSqlQuery *query, WickrBotProcessState* processState)
{
  QSqlRecord rec = query->record();

  processState->id            = query->value(rec.indexOf("id")).toInt();
  processState->process       = query->value(rec.indexOf("process")).toString();
  processState->process_id    = query->value(rec.indexOf("process_id")).toInt();
  processState->state         = query->value(rec.indexOf("state")).toInt();
  processState->last_update   = query->value(rec.indexOf("last_update")).toDateTime();

}


QSqlQueryModel *
WickrBotDatabase::getClientsModel() {
    QSqlQueryModel *model = new QSqlQueryModel(this);
    if (!initialized)
        return model;

    QString queryString = "SELECT id,name,port,interface,api_key,user,isHttps,sslKeyFile,sslCertFile,binary FROM clients";

    model->setQuery(queryString);
    return model;
}

bool
WickrBotDatabase::insertClientsRecord(WickrBotClients *client) {
    if (!initialized)
        return false;

    int id = getNextID(DB_CLIENTS_TABLE);

#if 0
    QString queryString = "INSERT INTO clients (id, name, port, interface, api_key, user, isHttps, sslKeyFile, sslCertFile) VALUES (?, '?', ?, '?', '?', '?', ?, '?', '?')";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);
    query.bindValue(1, client->name);
    query.bindValue(2, client->port);
    query.bindValue(3, client->iface);
    query.bindValue(4, client->api_key);
    query.bindValue(5, client->user);
    query.bindValue(6, client->isHttps ? 1 : 0);
    query.bindValue(7, client->sslKeyFile);
    query.bindValue(8, client->sslCertFile);
#else
    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO clients (id, name, port, interface, api_key, user, isHttps, sslKeyFile, sslCertFile, binary, integration_type) VALUES (%1, '%2', %3, '%4', '%5', '%6', %7, '%8', '%9', '%10', '%11')")
            .arg(id)
            .arg(client->name)
            .arg(client->port)
            .arg(client->iface)
            .arg(client->apiKey)
            .arg(client->user)
            .arg(client->isHttps ? 1 : 0)
            .arg(client->sslKeyFile)
            .arg(client->sslCertFile)
            .arg(client->binary)
            .arg(client->botType);
#endif
    if (!query.exec(queryString)) {
        qDebug() << query.lastQuery();
        QSqlError error = query.lastError();
        qDebug() << "insertClientsRecord: SQL error" << error;
        query.finish();
    } else {
        int numRows = query.numRowsAffected();
        query.finish();
        if (numRows > 0) {
            client->id = id;
            return true;
        }
    }
    return false;
}

bool
WickrBotDatabase::updateClientsRecord(WickrBotClients *client, bool insertIfNotExist) {
    if (!initialized)
        return false;

    // If the process state value does not exist then insert a new one
    WickrBotClients *checkClient = getClient(client->id);
    if (checkClient == NULL) {
        if (insertIfNotExist)
            return insertClientsRecord(client);
        else
            return false;
    } else {
        delete checkClient;
    }

#if 0
    QString queryString = "UPDATE clients SET name=?, port=?, interface=?, api_key=?, user=?, isHttps=?, sslKeyFile=?, sslCertFile=? WHERE id=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, client->name);
    query.bindValue(1, client->port);
    query.bindValue(2, client->iface);
    query.bindValue(3, client->apiKey);
    query.bindValue(4, client->user);
    query.bindValue(5, client->isHttps ? 1 : 0);
    query.bindValue(6, client->sslKeyFile);
    query.bindValue(7, client->sslCertFile);
    query.bindValue(8, client->id);
#else

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE clients SET name='%1', port=%2, interface='%3', api_key='%4', user='%5', isHttps=%6, sslKeyFile='%7', sslCertFile='%8', binary='%9', integration_type='%10' WHERE id='%11'")
            .arg(client->name)
            .arg(client->port)
            .arg(client->iface)
            .arg(client->apiKey)
            .arg(client->user)
            .arg(client->isHttps ? 1 : 0)
            .arg(client->sslKeyFile)
            .arg(client->sslCertFile)
            .arg(client->binary)
            .arg(client->botType)
            .arg(client->id);
#endif
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateClientsRecord: SQL error" << error;
        qDebug() << "queryString:" << queryString;
        query.finish();
        return false;
    }
    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

bool
WickrBotDatabase::deleteClientUsingName(QString name) {
    if (!initialized){
        return false;
    }

    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON;");

    QString queryString = QString("DELETE FROM clients WHERE name = '%1'").arg(name);
    if ( !query.exec(queryString)) {
        query.finish();
        qDebug() << "deleteClientUsingName: Could not delete record for" << name;
        qDebug() << "deleteClientUsingName: error=" << query.lastError();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}


/****************************************************************************************************************
 * Client Events handler functions
 ***************************************************************************************************************/

void
WickrBotDatabase::getClientEventFromQuery(QSqlQuery *query, WickrBotClientEvents *event)
{
    QSqlRecord rec = query->record();

    event->id = query->value(rec.indexOf("id")).toInt();
    event->m_clientID = query->value(rec.indexOf("client_id")).toInt();
    event->m_message = query->value(rec.indexOf("message")).toString();
    event->m_date = query->value(rec.indexOf("event_time")).toDateTime();
    event->m_type = (WickrBotClientEvents::WickrIOEventType)query->value(rec.indexOf("type")).toInt();
}


QList<WickrBotClientEvents *>
WickrBotDatabase::getClientEvents(int maxCount)
{
    QList<WickrBotClientEvents *> events;
    if (initialized) {
        QString queryString = "SELECT id,client_id,message,type,event_time FROM client_events";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->prepare(queryString);

        if ( !query->exec()) {
            qDebug() << "getClientEvents: Could not retrieve, Error=" << query->lastError();
        } else {
            int cur=0;
            while (query->next()) {
                WickrBotClientEvents *event = new WickrBotClientEvents();
                getClientEventFromQuery(query, event);
                events.append(event);
                if (cur++ >= maxCount)
                    break;
            }
        }
        query->finish();
        delete query;
    }
    return events;
}

bool
WickrBotDatabase::getClientEvent(int id, WickrBotClientEvents *event)
{
    bool retval=false;
    if (initialized) {
        QString queryString = "SELECT id,client_id,message,type,event_time FROM client_events WHERE id=?";
        QSqlQuery query(m_db);
        query.prepare(queryString);
        query.bindValue(0, id);

        if ( !query.exec()) {
            qDebug() << "getClientEvent: Could not retrieve, Error=" << query.lastError();
        } else {
            if (query.next()) {
                getClientEventFromQuery(&query, event);
                retval=true;
            }
        }
        query.finish();
    }
    return retval;
}

bool
WickrBotDatabase::insertClientEventRecord(int clientID, const QString& message, WickrBotClientEvents::WickrIOEventType type) {
    if (!initialized)
        return false;

    // The ProcessState will use local time since this has to operate independent of the server
    QString dateTime = getClientTime();

    int id = getNextID(DB_CLIENTEVENTS_TABLE);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO client_events (id, client_id, message, type, event_time) "
                  "VALUES (:id, :client_id, :message, :type, :event_time)");
    query.bindValue(":id", id);
    query.bindValue(":client_id", clientID);
    query.bindValue(":message", message);
    query.bindValue(":type", (int)type);
    query.bindValue(":event_time", dateTime);
    if (!query.exec()) {
        qDebug() << query.lastQuery();
        QSqlError error = query.lastError();
        qDebug() << "insertClientEventRecord: SQL error" << error;
        query.finish();
    } else {
        int numRows = query.numRowsAffected();
        query.finish();
        if (numRows > 0) {
            return true;
        }
    }
    return false;
}

bool
WickrBotDatabase::deleteClientEvent(int id) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "DELETE FROM client_events WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteClientEvent: Could not retrieve" << id << "last error=" << query.lastError();
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrBotDatabase::getFirstClientEvent(QString dateTime, WickrBotClientEvents *event) {
    bool retval=false;
    if (initialized) {
        QSqlQuery query(m_db);
        QString queryString = QString("SELECT MIN(id) FROM client_events WHERE runtime <= '%1'")
                .arg(dateTime);
        if ( !query.exec(queryString)) {
            qDebug() << "getFirstClientEvent: Could not retrieve action. DBError=" << query.lastError();
            query.finish();
        } else {
            if (query.next()) {
                int id = query.value(0).toInt();
                query.finish();
                retval = getClientEvent(id, event);
            } else {
                qDebug() << "getFirstClientEvent: No rows retrieved";
                query.finish();
            }
        }
    }
    return retval;
}



/****************************************************************************************************************
 * Statistics Table functions
 ***************************************************************************************************************/

void
WickrBotDatabase::getStatistic(QSqlQuery *query, WickrBotStatistics *stat)
{
    QSqlRecord rec = query->record();

    stat->id = query->value(rec.indexOf("id")).toInt();
    stat->clientID = query->value(rec.indexOf("client_id")).toInt();
    stat->statID = query->value(rec.indexOf("stat_id")).toInt(0);
    stat->statDesc = query->value(rec.indexOf("stat_desc")).toString();
    stat->statValue = query->value(rec.indexOf("stat_value")).toLongLong();
}

QList<WickrBotStatistics *>
WickrBotDatabase::getStatistics()
{
    QList<WickrBotStatistics *> stats;
    if (!initialized)
        return stats;

    QString queryString = "SELECT id,client_id,stat_id,stat_desc,stat_value FROM statistics";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);

    if ( !query->exec()) {
        qDebug() << "getStatistics: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            WickrBotStatistics *stat = new WickrBotStatistics();
            getStatistic(query, stat);
            stats.append(stat);
        }
    }
    query->finish();
    delete query;
    return stats;
}

QList<WickrBotStatistics *>
WickrBotDatabase::getClientStatistics(int clientID)
{
    QList<WickrBotStatistics *> stats;
    if (!initialized)
        return stats;

    QString queryString = "SELECT id,client_id,stat_id,stat_desc,stat_value FROM statistics WHERE client_id=?";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);
    query->bindValue(0, clientID);

    if ( !query->exec()) {
        qDebug() << "getClientStatistics: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            WickrBotStatistics *stat = new WickrBotStatistics();
            getStatistic(query, stat);
            stats.append(stat);
        }
    }
    query->finish();
    delete query;
    return stats;
}

bool
WickrBotDatabase::deleteClientStatistics(int clientID)
{
    if (!initialized){
        return false;
    }

    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON;");

    QString queryString = QString("DELETE FROM statistics WHERE client_id=?");
    query.prepare(queryString);
    query.bindValue(0, clientID);
    if ( !query.exec()) {
        query.finish();
        qDebug() << "deleteClientStatistics: Could not delete statistics for" << clientID;
        qDebug() << "deleteClientStatistics: error=" << query.lastError();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}


bool
WickrBotDatabase::insertStatistic(int clientID, int statID, const QString &statDesc, qlonglong statValue) {
    if (!initialized)
        return false;
    bool retval = false;

    int id = getNextID(DB_STATISTICS_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO statistics (id, client_id, stat_id, stat_desc, stat_value) VALUES (%1, %2, %3, '%4', %5)")
            .arg(id)
            .arg(clientID)
            .arg(statID)
            .arg(statDesc)
            .arg(statValue);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertStatistic: SQL error" << error;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrBotDatabase::incStatistic(int clientID, int statID, const QString &statDesc, qlonglong increment)
{
    if (!initialized)
        return false;

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE statistics SET stat_value = stat_value + %1 WHERE client_id=%2 and stat_id=%3 and stat_desc='%4'")
            .arg(increment)
            .arg(clientID)
            .arg(statID)
            .arg(statDesc);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "incStatistic: SQL error" << error;
        query.finish();
        return false;
    }
    int numRows = query.numRowsAffected();
    query.finish();

    if (numRows == 0) {
        return insertStatistic(clientID, statID, statDesc, increment);
    }
    return true;
}



/****************************************************************************************************************
 * Misc. database functions
 ***************************************************************************************************************/

/**
 * @brief WickrBotDatabase::getNextID
 * This method will get the next ID value to use when creating a new record in the
 * table identified by the input table value.
 * @param table
 * @return -1 is returned if there is a failure or the db is not initialized. Otherwise the next ID is returned.
 */
int
WickrBotDatabase::getNextID(const QString &table) {
    if (!initialized)
        return -1;

    QString queryString = "SELECT MAX(id) FROM " + table;
    QSqlQuery query(m_db);
    query.prepare(queryString);

    if ( !query.exec()) {
        qDebug() << "getNextID failed SQL query on table" << table;
        qDebug() << "getNextID: last error" << query.lastError();
        query.finish();
        return -1;
    }

    if (query.next()) {
        int max = query.value(0).toInt(0);
        query.finish();
        return max+1;
    }
    query.finish();
    return 1;
}

/**
 * @brief WickrBotDatabase::touchRecord
 * This method will update the "lastaccess" datetime value for the record with the input ID
 * from the input table.
 * @param id ID of the record to update
 * @param table Table name that contains the record to update
 * @return true is returned if the record is updated, false if not or failure
 */
bool
WickrBotDatabase::touchRecord(int id, const QString &table) {
    if (!initialized)
        return false;

    // All lastaccess fields are based on the client's time, not the server time
    QString dateTime = getClientTime();
    qDebug() << "Touching" << table << "record id" << id << ", date is " << dateTime;

    QString queryString = QString("UPDATE %1 SET lastaccess = '%2'' WHERE id = %3")
            .arg(table)
            .arg(dateTime)
            .arg(id);
    QSqlQuery query(m_db);

    if ( !query.exec(queryString)) {
        qDebug() << "Could not update" << table << "with ID=" << id;
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/**
 * @brief WickrBotDatabase::size
 * This function will return the size of the database.
 * @return Integer value size of the database
 */
int
WickrBotDatabase::size()
{
    if (this->m_db.databaseName().size() <= 0)
        return 0;
    QFileInfo infol(this->m_db.databaseName());
    return infol.size();
}

/****************************************************************************************************************
 * Misc. functions
 ***************************************************************************************************************/

/**
 * @brief getClientTime
 * This functio nwill return the string version of the Client's currently known date and time.
 * The format is based on the DB_DATETIME_FORAMT defined in the header file.
 * @return Formatted string version of teh client's date and time
 */
QString WickrBotDatabase::getClientTime()
{
    QDateTime dt = QDateTime::currentDateTime();
    QString dateTime = dt.toString(DB_DATETIME_FORMAT);
    return dateTime;
}

QString WickrBotDatabase::getClientTimeWithOffset(long seconds)
{
    quint64 secs(seconds);

    QDateTime dt = QDateTime::currentDateTime();
    QDateTime newDt = dt.addSecs(secs);
    QString dateTime = newDt.toString(DB_DATETIME_FORMAT);
    return dateTime;
}

/**
 * @brief WickrDBAdapter::dumpTableCounts
 * Dump stats about the database table counts
 * @return
 */
QString
WickrBotDatabase::dumpTableCounts()
{
    QString retString;

    if ( m_db.isOpen() ) {
        QSqlQuery chk(m_db);

        QStringList list;
        QList<QString> tableList;
        tableList << QString("attachment_cache") <<
                              QString("clients") <<
                              QString("action_cache") <<
                              QString("process_state") <<
                              QString("last_user_message");

        for (QString table : tableList) {
            QString query ("SELECT COUNT(*) from " + table);
            chk.prepare(query);
            chk.exec();

            if ( chk.isActive() ) {
                if (chk.next()) {
                    int count = chk.value(0).toInt();
                    QString cur = QString("Count for %1 is %2").arg(table).arg(count);
                    list.append(cur);
//                    qDebug() << "Count for " << table << " is " << count;
                }
            } else {
                qDebug() << "Error get count for " << table << "," << m_db.lastError().number() << m_db.lastError().text();
            }

            chk.finish();
        }
        retString = list.join("\n");
    }
    return retString;
}

/**
 * @brief WickrBotActionDatabase::getFirstAction
 * This method will retrieve the first action in the database that has a runtime value less than
 * the current time. The input is a pointer to an WickrBotActionCache record to be populated with
 * the contents of the retrieved record.
 * @param action Pointer to the record to populate
 * @return true if a record is found, false if not
 */
bool
WickrBotDatabase::getFirstAction(WickrBotDatabase *botDB, const QString& dateTime, WickrBotActionCache *action) {
    // Actions from the server are based on the server's time
//    QString dateTime = getServerTime();

    return botDB->getFirstAction(dateTime, action);
}

bool
WickrBotDatabase::getFirstAction(WickrBotDatabase *botDB, const QString& dateTime, WickrBotActionCache *action, int clientID) {
    // Actions from the server are based on the server's time
//    QString dateTime = getServerTime();

    return botDB->getFirstAction(dateTime, action, clientID);
}

        #if 0
/**
 * @brief getServerTime
 * This function will return the string version of the Server's currently known date and time.
 * The format is based on the DB_DATETIME_FORAMT defined in the header file
 * @return Formatted string version of server's date and time
 */
QString WickrBotDatabase::getServerTime()
{
    long currentTime = WickrCore::WickrSession::getActiveSession()->getLocalAppClock()->getCurrentTime();
    QDateTime dt = QDateTime::fromTime_t((uint)currentTime);
    QDateTime dt2 = dt.toUTC();
    QString dateTime = dt2.toString(DB_DATETIME_FORMAT);
//    QString dateTime = dt.toString(DB_DATETIME_FORMAT);
    return dateTime;
}
#endif
