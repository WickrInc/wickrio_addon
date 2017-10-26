#include <QtSql>
#include <QDebug>
#include <QDateTime>
#include "wickriodatabase.h"

/**
 * @brief WickrIOClientDatabase::WickrIOClientDatabase
 * Constructor for the WickrIOClientDatabase class. This constructor will call internal methods to setup
 * and initialize the WickrBot database.
 */
WickrIOClientDatabase::WickrIOClientDatabase(const QString &dirPath) : WickrBotClientDatabase(dirPath)
{
    createRelationalTables();
}

/**
 * @brief WickrBotDatabase::createRelationalTables
 * This method will create the tables for the WickrBot database.
 * @return True is returned if created successfully and fals if not
 */
bool
WickrIOClientDatabase::createRelationalTables()
{
    // Check if the Application Settings table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_APPSETTINGS_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE app_settings (id int primary key, "
                        "client_id int NOT NULL, "
                        "type text, "
                        "value text NOT NULL, "
                        "FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE)")) {
            qDebug() << "create app_settings table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the Console Users table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_CONSOLEUSERS_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE console_users (id int primary key, user text UNIQUE NOT NULL, password text NOT NULL, permissions int, max_clients int, auth_type int, email text)")) {
            qDebug() << "create console_users table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Need to extend the clients table to contain a reference to the Console Users
    QSqlQuery tstQuery(m_db);
    tstQuery.prepare("SELECT console_id from clients");
    tstQuery.exec();
    if (!tstQuery.isActive()) {
        if (!runSimpleQuery("ALTER TABLE clients ADD console_id REFERENCES console_users (id)")) {
            qDebug() << "Could not alter clients table!";
        }
    }
    tstQuery.finish();

    // Check if the Tokens table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_TOKEN_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE tokens (id int primary key, "
                        "console_id int UNIQUE NOT NULL, "
                        "token text UNIQUE NOT NULL, "
                        "remote text, "
                        "FOREIGN KEY (console_id) REFERENCES console_users(id) ON DELETE CASCADE)")) {
            qDebug() << "create tokens table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    // Check if the Message table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_MESSAGES_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE messages (id int primary key, "
                        "client_id int NOT NULL, "
                        "timestamp int NOT NULL, "
                        "type int, "
                        "json text NOT NULL, "
                        "has_attachment int, "
                        "FOREIGN KEY (client_id) REFERENCES clients(id) ON DELETE CASCADE)")) {
            qDebug() << "create messages table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();

        QSqlQuery indexQuery(m_db);
        if (!indexQuery.exec("CREATE INDEX messages_time_index ON messages (client_id, timestamp)")) {
            qDebug() << "create messages_time_index index failed, query error=" << query.lastError();
            indexQuery.finish();
            return false;
        }
        indexQuery.finish();
    }

    // Check if the Attachments table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_ATTACHMENTS_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("CREATE TABLE attachments (id int primary key, "
                        "message_id int NOT NULL, "
                        "filename text NOT NULL, "
                        "realfilename text NOT NULL, "
                        "FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE)")) {
            qDebug() << "create attachments table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }

    return true;
}

/****************************************************************************************************************
 * App Settings (app_settings) handler functions
 ***************************************************************************************************************/

bool
WickrIOClientDatabase::insertAppSetting(int clientID, const QString &type, const QString &value) {
    if (!initialized)
        return false;
    bool retval = false;

    int id = getNextID(DB_APPSETTINGS_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO app_settings (id, client_id, type, value) VALUES (%1, %2, '%3', '%4')")
            .arg(id)
            .arg(clientID)
            .arg(type)
            .arg(value);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertAppSetting: SQL error" << error;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}


bool
WickrIOClientDatabase::updateAppSetting(WickrIOAppSettings *appSetting) {
    if (!initialized)
        return false;
    bool retval = false;

    WickrIOAppSettings setting;

    // If the process state value does not exist then insert a new one
    if (! getAppSetting(appSetting->clientID, appSetting->type, &setting)) {
        return insertAppSetting(appSetting->clientID, appSetting->type, appSetting->value);
    } else {
        appSetting->id = setting.id;
    }

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE app_settings SET type='%1', value='%2', client_id='%3' WHERE id='%4'")
            .arg(appSetting->type)
            .arg(appSetting->value)
            .arg(appSetting->clientID)
            .arg(appSetting->id);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateAppSetting: SQL error" << error;
    } else  if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getAppSetting(int id, WickrIOAppSettings *appSetting) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT client_id,type,value FROM app_settings WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getAppSetting: Could not retrieve" << id;
    } else if (query.next()) {
        appSetting->id = id;
        appSetting->clientID = query.value(0).toInt();
        appSetting->type = query.value(1).toString();
        appSetting->value = query.value(2).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getAppSetting(int clientID, QString type, WickrIOAppSettings *appSetting) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT id,value FROM app_settings WHERE client_id = ? and type = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, clientID);
    query.bindValue(1, type);

    if ( !query.exec()) {
        qDebug() << "getAppSetting: Could not retrieve, for client_id=" << clientID << ", type=" << type;
    } else if (query.next()) {
        appSetting->id = query.value(0).toInt();
        appSetting->clientID = clientID;
        appSetting->type = type;
        appSetting->value = query.value(1).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::deleteAppSetting(int id) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM app_settings WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteAppSetting: Could not retrieve" << id << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/****************************************************************************************************************
 * Console Users (console_users) handler functions
 ***************************************************************************************************************/

bool
WickrIOClientDatabase::insertConsoleUser(WickrIOConsoleUser *cuser)
{
    return insertConsoleUser(cuser->user, cuser->password, cuser->permissions, cuser->maxclients, cuser->authType, cuser->email);
}

bool
WickrIOClientDatabase::insertConsoleUser(const QString &user, const QString &password, int permissions, int max_clients, int authType, QString email) {
    if (!initialized)
        return false;
    bool retval = false;

    int id = getNextID(DB_CONSOLEUSERS_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO console_users (id, user, password, permissions, max_clients, auth_type, email) VALUES (%1, '%2', '%3', %4, %5, %6, '%7')")
            .arg(id)
            .arg(user)
            .arg(password)
            .arg(permissions)
            .arg(max_clients)
            .arg(authType)
            .arg(email);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertConsoleUser: SQL error" << error;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}


bool
WickrIOClientDatabase::updateConsoleUser(WickrIOConsoleUser *cUser) {
    if (!initialized)
        return false;
    bool retval = false;

    WickrIOConsoleUser temp;

    // If the process state value does not exist then insert a new one
    if (! getConsoleUser(cUser->user, &temp)) {
        return insertConsoleUser(cUser->user, cUser->password, cUser->permissions, cUser->maxclients, cUser->authType, cUser->email);
    } else {
        cUser->id = temp.id;
    }

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE console_users SET user='%1', password='%2', permissions=%3, max_clients=%4 WHERE id='%5'")
            .arg(cUser->user)
            .arg(cUser->password)
            .arg(cUser->permissions)
            .arg(cUser->maxclients)
            .arg(cUser->id);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateConsoleUser: SQL error" << error;
    } else  if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

/**
 * @brief WickrIOClientDatabase::getConsoleUsers
 * Return a complete list of all of the Console Users defined in the database.
 * The list is a QList of pointers to WickrIOConsoleUser objects. It is up to
 * the caller to free the memory associated with the returned objects.
 * @return QList of WickrIOConsoleUser object pointers
 */
QList<WickrIOConsoleUser *>
WickrIOClientDatabase::getConsoleUsers() {
    QList<WickrIOConsoleUser *> retval;
    if (initialized) {
        QString queryString = "SELECT id,user,password,permissions,max_clients,auth_type,email FROM console_users";
        QSqlQuery query(m_db);
        query.prepare(queryString);

        if ( !query.exec()) {
            qDebug() << "getConsoleUsers: Failed query" << query.lastError();
        } else {
            while (query.next()) {
                WickrIOConsoleUser *cUser = new WickrIOConsoleUser();
                cUser->id = query.value(0).toInt();
                cUser->user = query.value(1).toString();
                cUser->password = query.value(2).toString();
                cUser->permissions = query.value(3).toInt();
                cUser->maxclients = query.value(4).toInt();
                cUser->authType = query.value(5).toInt();
                cUser->email = query.value(6).toString();

                retval.append(cUser);
            }
        }
        query.finish();
    }
    return retval;
}

int
WickrIOClientDatabase::numConsoleUsers() {
    int retval = 0;
    if (initialized) {
        QString queryString = "SELECT COUNT( * ) FROM console_users";
        QSqlQuery query(m_db);
        query.prepare(queryString);

        if ( !query.exec()) {
            qDebug() << "getConsoleUsers: Failed query" << query.lastError();
        } else {
            if (query.next()) {
                retval = query.value(0).toInt();
            }
        }
        query.finish();
    }
    return retval;
}

bool
WickrIOClientDatabase::getConsoleUser(int id, WickrIOConsoleUser *cUser) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT user,password,permissions,max_clients,auth_type,email FROM console_users WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getConsoleUser: Could not retrieve" << id;
    } else if (query.next()) {
        cUser->id = id;
        cUser->user = query.value(0).toString();
        cUser->password = query.value(1).toString();
        cUser->permissions = query.value(2).toInt();
        cUser->maxclients = query.value(3).toInt();
        cUser->authType = query.value(4).toInt();
        cUser->email = query.value(5).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getConsoleUser(const QString &user, WickrIOConsoleUser *cUser) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT id,password,permissions,max_clients,auth_type,email FROM console_users WHERE user = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, user);

    if ( !query.exec()) {
        qDebug() << "getConsoleUser: Could not retrieve, for user=" << user << query.lastError();
    } else if (query.next()) {
        cUser->id = query.value(0).toInt();
        cUser->user = user;
        cUser->password = query.value(1).toString();
        cUser->permissions = query.value(2).toInt();
        cUser->maxclients = query.value(3).toInt();
        cUser->authType = query.value(4).toInt();
        cUser->email = query.value(5).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::deleteConsoleUser(int id) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM console_users WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteConsoleUser: Could not delete" << id << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

bool
WickrIOClientDatabase::deleteConsoleUser(const QString &user) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM console_users WHERE user = ?";
    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON;");
    query.prepare(queryString);
    query.bindValue(0, user);

    if ( !query.exec()) {
        qDebug() << "deleteConsoleUser: Could not delete" << user << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

/****************************************************************************************************************
 * Functions to handle the extended clients table
 ***************************************************************************************************************/

QList<WickrIOClients *>
WickrIOClientDatabase::getClients()
{
    QList<WickrIOClients *> clients;
    if (!initialized)
        return clients;

    QString queryString = "SELECT id,name,port,interface,api_key,user,password,isHttps,sslKeyFile,sslCertFile,console_id,binary FROM clients";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);

    if ( !query->exec()) {
        qDebug() << "getClients: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            WickrIOClients *client = new WickrIOClients();
            getClient(query, client);
            QSqlRecord rec = query->record();
            client->console_id = query->value(rec.indexOf("console_id")).toInt();
            clients.append(client);
        }
    }
    query->finish();
    delete query;
    return clients;
}

WickrIOClients *
WickrIOClientDatabase::getClientUsingName(QString name)
{
    WickrIOClients * client = NULL;

    if (!initialized)
        return client;

    QString queryString = "SELECT id,name,port,interface,api_key,user,password,isHttps,sslKeyFile,sslCertFile,console_id,binary FROM clients WHERE name=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, name);

    if ( !query.exec()) {
        qDebug() << "getClientUsingName: Could not retrieve, Error=" << query.lastError();
    } else {
        if (query.next()) {
            client = new WickrIOClients();
            QSqlRecord rec = query.record();
            client->console_id = query.value(rec.indexOf("console_id")).toInt();
            getClient(&query, client);
        }
    }
    query.finish();
    return client;
}

int
WickrIOClientDatabase::getClientsConsoleID(int id)
{
    int retval = 0;
    if (initialized) {
        QString queryString = "SELECT console_id FROM clients WHERE id=?";
        QSqlQuery query(m_db);
        query.prepare(queryString);
        query.bindValue(0, id);

        if ( !query.exec()) {
            qDebug() << "getClientsConsoleID: Could not retrieve, Error=" << query.lastError();
        } else {
            if (query.next()) {
                retval = query.value(0).toInt();
            }
        }
        query.finish();
    }
    return retval;
}

/**
 * @brief WickrIOClientDatabase::getConsoleClients
 * This function will get the list of clients that are associated with the input console ID
 * @param console_id
 * @return
 */
QList<WickrIOClients *>
WickrIOClientDatabase::getConsoleClients(int console_id)
{
    QList<WickrIOClients *> clients;
    if (!initialized)
        return clients;

    QString queryString = "SELECT id,name,port,interface,api_key,user,password,isHttps,sslKeyFile,sslCertFile,binary FROM clients WHERE console_id = ?";
    QSqlQuery *query = new QSqlQuery(m_db);
    query->prepare(queryString);
    query->bindValue(0, console_id);

    if ( !query->exec()) {
        qDebug() << "getConsoleClients: Could not retrieve, Error=" << query->lastError();
    } else {
        while (query->next()) {
            WickrIOClients *client = new WickrIOClients();
            getClient(query, client);
            clients.append(client);
        }
    }
    query->finish();
    delete query;
    return clients;
}

int
WickrIOClientDatabase::numConsoleClients(int console_id)
{
    int retval = 0;
    if (initialized) {
        QString queryString = "SELECT COUNT( * ) FROM clients WHERE console_id = ?";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->prepare(queryString);
        query->bindValue(0, console_id);

        if ( !query->exec()) {
            qDebug() << "numConsoleClients: Could not retrieve, Error=" << query->lastError();
        } else {
            if (query->next()) {
                retval = query->value(0).toInt();
            }
        }
        query->finish();
        delete query;
    }
    return retval;
}

/**
 * @brief WickrIOClientDatabase::getClientsConsoleUser
 * This function will get the username of the console user that is associated with
 * the input client's ID.  An SQL join is performed to make the relationship
 * between the input client and the associated console user record.
 * @param id
 * @return
 */
QString
WickrIOClientDatabase::getClientsConsoleUser(int id)
{
    QString retuser;

    if (initialized) {
        QString queryString = "SELECT console_users.user FROM console_users INNER JOIN clients WHERE clients.id=? AND console_users.id = clients.console_id";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->prepare(queryString);
        query->bindValue(0, id);

        if ( !query->exec()) {
            qDebug() << "getClientsConsoleUser: Could not retrieve, Error=" << query->lastError();
        } else if (query->next()) {
            retuser = query->value(0).toString();
        }
        query->finish();
        delete query;
    }
    return retuser;
}

/**
 * @brief WickrIOClientDatabase::deleteClientsOfConsoleUser
 * This function will delete all of the clients records that are related to the input
 * console users id.
 * @param cUserID
 * @return
 */
bool
WickrIOClientDatabase::deleteClientsOfConsoleUser(int cUserID)
{
    bool retval = false;

    if (initialized) {
        QString queryString = "DELETE FROM clients WHERE console_id = ?";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->exec("PRAGMA foreign_keys = ON;");
        query->prepare(queryString);
        query->bindValue(0, cUserID);

        if ( !query->exec()) {
            qDebug() << "deleteClientsOfConsoleUser: Could not delete clients of console user, Error=" << query->lastError();
        } else {
            int numRows = query->numRowsAffected();
            retval = (numRows > 0);
        }
        query->finish();
        delete query;
    }
    return retval;
}

/**
 * @brief WickrIOClientDatabase::clearClientsOfConsoleUser
 * This function will set the console users id to 0 for all of the clients that
 * are associated with the input console users ID value.
 * @param cUserID
 * @return
 */
bool
WickrIOClientDatabase::clearClientsOfConsoleUser(int cUserID)
{
    bool retval = false;

    if (initialized) {
        QString queryString = "UPDATE clients SET console_id = 0 WHERE console_id = ?";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->prepare(queryString);
        query->bindValue(0, cUserID);

        if ( !query->exec()) {
            qDebug() << "clearClientsOfConsoleUser: Could not update clients of console user, Error=" << query->lastError();
        } else {
            int numRows = query->numRowsAffected();
            retval = (numRows > 0);
        }
        query->finish();
        delete query;
    }
    return retval;
}

bool
WickrIOClientDatabase::insertClientsRecord(WickrIOClients *client) {
    bool retval = false;
    if (initialized) {
        if (WickrBotDatabase::insertClientsRecord(client)) {
            if (client->console_id > 0) {
                // Update the wickrIO fields of the client record
                QString queryString = "UPDATE clients SET console_id = ? WHERE id = ?";
                QSqlQuery *query = new QSqlQuery(m_db);
                query->prepare(queryString);
                query->bindValue(0, client->console_id);
                query->bindValue(1, client->id);

                if ( !query->exec()) {
                    qDebug() << "updateClientsRecord: Could not update clients of console user, Error=" << query->lastError();
                } else {
                    int numRows = query->numRowsAffected();
                    retval = (numRows > 0);
                }
                query->finish();
                delete query;
            } else {
                retval = true;
            }
        }
    }

    return retval;
}


/**
 * @brief WickrIOClientDatabase::updateClientsRecord
 * This function extends the WickrBotDatabase updateClientsRecord() function to
 * allow setting the console_id field
 * @param client
 * @param insertIfNotExist
 * @return
 */
bool
WickrIOClientDatabase::updateClientsRecord(WickrIOClients *client, bool insertIfNotExist) {
    bool retval = false;
    if (WickrBotDatabase::updateClientsRecord(client, insertIfNotExist)) {
        QString queryString = "UPDATE clients SET console_id = ? WHERE id = ?";
        QSqlQuery *query = new QSqlQuery(m_db);
        query->prepare(queryString);
        query->bindValue(0, client->console_id);
        query->bindValue(1, client->id);

        if ( !query->exec()) {
            qDebug() << "updateClientsRecord: Could not update clients of console user, Error=" << query->lastError();
        } else {
            int numRows = query->numRowsAffected();
            retval = (numRows > 0);
        }
        query->finish();
        delete query;
    }
    return retval;
}


/****************************************************************************************************************
 * Functions to handle the parsers
 ***************************************************************************************************************/

QList<WickrIOParsers *>
WickrIOClientDatabase::getParsers()
{
    QList<WickrIOParsers *> parsers;
    if (!initialized)
        return parsers;

    qDebug() << "TODO: implement getParsers()";
    return parsers;
}



/****************************************************************************************************************
 * Tokens (tokens) handler functions
 ***************************************************************************************************************/

bool
WickrIOClientDatabase::insertToken(WickrIOTokens *token)
{
    return insertToken(token->token, token->console_id, token->remote);
}

bool
WickrIOClientDatabase::insertToken(const QString &token, int console_id, const QString &remote) {
    if (!initialized)
        return false;
    bool retval = false;

    int id = getNextID(DB_TOKEN_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO tokens (id, token, console_id, remote) VALUES (%1, '%2', %3, '%4')")
            .arg(id)
            .arg(token)
            .arg(console_id)
            .arg(remote);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertToken: SQL error" << error;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}


bool
WickrIOClientDatabase::updateToken(WickrIOTokens *token) {
    if (!initialized)
        return false;
    bool retval = false;

    WickrIOTokens temp;

    // If the process state value does not exist then insert a new one
    if (! getToken(token->id, &temp)) {
        return insertToken(token->token, token->console_id, token->remote);
    } else {
        token->id = temp.id;
    }

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE tokens SET token='%1', remote='%2', console_id=%3 WHERE id='%4'")
            .arg(token->token)
            .arg(token->remote)
            .arg(token->console_id)
            .arg(token->id);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateToken: SQL error" << error;
    } else  if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

/**
 * @brief WickrIOClientDatabase::getConsoleUsers
 * Return a complete list of all of the Console Users defined in the database.
 * The list is a QList of pointers to WickrIOConsoleUser objects. It is up to
 * the caller to free the memory associated with the returned objects.
 * @return QList of WickrIOConsoleUser object pointers
 */
QList<WickrIOTokens *>
WickrIOClientDatabase::getTokens() {
    QList<WickrIOTokens *> retval;
    if (initialized) {
        QString queryString = "SELECT id,token,console_id,remote FROM tokens";
        QSqlQuery query(m_db);
        query.prepare(queryString);

        if ( !query.exec()) {
            qDebug() << "getConsoleUsers: Failed query" << query.lastError();
        } else {
            while (query.next()) {
                WickrIOTokens *token = new WickrIOTokens();
                token->id = query.value(0).toInt();
                token->token = query.value(1).toString();
                token->console_id = query.value(2).toInt();
                token->remote = query.value(3).toString();
                retval.append(token);
            }
        }
        query.finish();
    }
    return retval;
}

bool
WickrIOClientDatabase::getToken(int id, WickrIOTokens *token) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT token,console_id,remote FROM tokens WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getToken: Could not retrieve" << id;
    } else if (query.next()) {
        token->id = id;
        token->token = query.value(0).toString();
        token->console_id = query.value(1).toInt();
        token->remote = query.value(2).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getToken(const QString &tokenStr, WickrIOTokens *token) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT id,console_id,remote FROM tokens WHERE token = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, tokenStr);

    if ( !query.exec()) {
        qDebug() << "getToken: Could not retrieve, for token=" << tokenStr << query.lastError();
    } else if (query.next()) {
        token->id = query.value(0).toInt();
        token->token = tokenStr;
        token->console_id = query.value(1).toInt();
        token->remote = query.value(2).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::deleteToken(int id) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM tokens WHERE id = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteToken: Could not delete" << id << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

bool
WickrIOClientDatabase::deleteToken(const QString &token) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM tokens WHERE token = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, token);

    if ( !query.exec()) {
        qDebug() << "deleteToken: Could not delete" << token << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

bool
WickrIOClientDatabase::getTokenConsoleUser(const QString &token, WickrIOConsoleUser *consoleUser) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT console_users.id,user,password,permissions,max_clients,auth_type,email FROM console_users INNER JOIN tokens WHERE tokens.token=? AND console_users.id = tokens.console_id";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, token);

    if ( !query.exec()) {
        qDebug() << "getTokenConsoleUser: Could not retrieve, for user=" << token << query.lastError();
    } else if (query.next()) {
        consoleUser->id = query.value(0).toInt();
        consoleUser->user = query.value(1).toString();
        consoleUser->password = query.value(2).toString();
        consoleUser->permissions = query.value(3).toInt();
        consoleUser->maxclients = query.value(4).toInt();
        consoleUser->authType = query.value(5).toInt();
        consoleUser->email = query.value(6).toString();
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getConsoleUserToken(int id, WickrIOTokens *token) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = "SELECT id,token,remote FROM tokens WHERE console_id=?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "getConsoleUserToken: Could not retrieve, for user=" << token << query.lastError();
    } else if (query.next()) {
        token->id = query.value(0).toInt();
        token->token = query.value(1).toString();
        token->remote = query.value(2).toString();
        token->console_id = id;
        retval = true;
    }
    query.finish();
    return retval;
}


/****************************************************************************************************************
 * Messages (messages) handler functions
 ***************************************************************************************************************/

int
WickrIOClientDatabase::insertMessage(long timestamp, int clientID, const QString &json, int type, int hasAttachment) {
    int id = -1;
    if (initialized) {
        id = getNextID(DB_MESSAGES_TABLE);

        QSqlQuery query(m_db);
        query.prepare("INSERT INTO messages (id, timestamp, client_id, type, json, has_attachment) VALUES (:id, :timestamp, :clientID, :type, :json, :hasAttachment)");
        query.bindValue(":id", id);
        query.bindValue(":timestamp", (int)timestamp);
        query.bindValue(":clientID", clientID);
        query.bindValue(":type", type);
        query.bindValue(":json", json);
        query.bindValue(":hasAttachment", hasAttachment);

        if (!query.exec()) {
            QSqlError error = query.lastError();
            qDebug() << "insertMessage: SQL error" << error;
            id = -1;
        } else if (query.numRowsAffected() == 0) {
            id = -1;
        }
        query.finish();
    }
    return id;
}


bool
WickrIOClientDatabase::updateMessage(WickrIOMessage *message) {
    if (!initialized)
        return false;
    bool retval = false;
    int hasAttachment = message->hasAttachment ? 1 : 0;

    WickrIOMessage temp;

    // If the process state value does not exist then insert a new one
    if (! getMessage(message->id, &temp)) {
        return insertMessage(message->timestamp, message->clientID, message->json, message->type, hasAttachment);
    }

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE messages SET timestamp=%1, type=%2, json='%3', client_id='%4', has_attachment='%5' WHERE id='%5'")
            .arg(message->timestamp)
            .arg(message->type)
            .arg(message->json)
            .arg(message->clientID)
            .arg(message->id)
            .arg(hasAttachment);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "updateMessage: SQL error" << error;
    } else  if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

bool
WickrIOClientDatabase::getMessage(int id, WickrIOMessage *message) {
    if (!initialized)
        return false;
    bool retval = false;

    QString queryString = QString("SELECT timestamp,client_id,type,json,has_attachment FROM messages WHERE id = %1").arg(id);
    QSqlQuery query(m_db);
    query.prepare(queryString);

    if ( !query.exec()) {
        qDebug() << "getMessage: Could not retrieve" << id;
    } else if (query.next()) {
        message->id = id;
        message->timestamp = (long)query.value(0).toInt();
        message->clientID = query.value(1).toInt();
        message->type = query.value(2).toInt();
        message->json = query.value(3).toString();
        message->hasAttachment = query.value(4).toInt()!= 0 ? true : false;
        retval = true;
    }
    query.finish();
    return retval;
}

QList<int>
WickrIOClientDatabase::getMessageIDs(int clientID) {
    QList<int> ids;
    if (!initialized)
        return ids;

    QString queryString = QString("SELECT id FROM messages WHERE client_id = %1 ORDER BY timestamp ASC").arg(clientID);
    QSqlQuery query(m_db);
    query.prepare(queryString);

    if ( !query.exec()) {
        qDebug() << "getMessageIDs: Could not retrieve for client id" << clientID << query.lastError();
    } else {
        while (query.next()) {
            ids.append(query.value(0).toInt());
        }
    }
    query.finish();
    return ids;
}

bool
WickrIOClientDatabase::messageHasAttachments(int id) {
    bool retval = false;
    if (initialized) {

        QString queryString = QString("SELECT has_attachment FROM messages WHERE id = %1").arg(id);
        QSqlQuery query(m_db);
        query.prepare(queryString);

        if ( !query.exec()) {
            qDebug() << "messageHasAttachments: Could not retrieve for id" << id << query.lastError();
        } else {
            if (query.next()) {
                retval = (query.value(0).toInt() != 0) ? true : false;
            }
        }
        query.finish();
    }
    return retval;
}

bool
WickrIOClientDatabase::deleteMessage(int id) {
    if (!initialized)
        return false;

    // If there are attachments then delete the files!
    QList<WickrIODBAttachment *> attachments = getAttachments(id);
    for (WickrIODBAttachment *attach : attachments) {
        QFile tempFile(attach->m_filename);
        tempFile.remove();
        delete attach;
    }

    QString queryString = "DELETE FROM messages WHERE id = ?";
    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON;");
    query.prepare(queryString);
    query.bindValue(0, id);

    if ( !query.exec()) {
        qDebug() << "deleteMessage: Could not delete" << id << "last error=" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

int
WickrIOClientDatabase::getClientsOutMessagesCount(int clientID)
{
    int retVal = 0;

    if (!initialized)
        return retVal;

    QString queryString = "SELECT count(id) FROM messages WHERE client_id=?";
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
 * Message attachments (attachments) handler functions
 ***************************************************************************************************************/

bool
WickrIOClientDatabase::insertAttachment(int messageID, const QString &filename, const QString &realFilename) {
    if (!initialized)
        return false;
    bool retval = false;

    int id = getNextID(DB_ATTACHMENTS_TABLE);

    QSqlQuery query(m_db);
    QString queryString = QString("INSERT INTO attachments (id, message_id, filename, realfilename) VALUES (%1, %2, '%3', '%4')")
            .arg(id)
            .arg(messageID)
            .arg(filename)
            .arg(realFilename);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertAttachment: SQL error" << error;
    } else if (query.numRowsAffected() > 0) {
        retval = true;
    }
    query.finish();
    return retval;
}

QList<WickrIODBAttachment *>
WickrIOClientDatabase::getAttachments(int messageID) {
    QList<WickrIODBAttachment *> files;
    if (!initialized)
        return files;

    QString queryString = QString("SELECT filename, realfilename FROM attachments WHERE message_id = %1").arg(messageID);
    QSqlQuery query(m_db);
    query.prepare(queryString);

    if ( !query.exec()) {
        qDebug() << "getAttachments: Could not retrieve for message id" << messageID << query.lastError();
    } else {
        while (query.next()) {
            WickrIODBAttachment *att = new WickrIODBAttachment(query.value(0).toString(), query.value(1).toString());
            files.append(att);
        }
    }
    query.finish();
    return files;
}

