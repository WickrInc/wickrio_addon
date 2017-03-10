#include <QtSql>
#include <QDebug>
#include <QDateTime>
#include "wickrbotclientdatabase.h"
#include "wickrbotactioncache.h"
#include "wickrbotprocessstate.h"
#include "wickrbotclients.h"

/**
 * @brief WickrBotClientDatabase::WickrBotClientDatabase
 * Constructor for the WickrBotClientDatabase class. This constructor will call internal methods to setup
 * and initialize the WickrBot database.
 */
WickrBotClientDatabase::WickrBotClientDatabase(const QString &dirPath) : WickrBotDatabase(dirPath)
{
    createRelationalTables();
}

/**
 * @brief WickrBotDatabase::createRelationalTables
 * This method will create the tables for the WickrBot database.
 * @return True is returned if created successfully and fals if not
 */
bool
WickrBotClientDatabase::createRelationalTables()
{
    // Check if the attachment cache table exists already, if not create it
    if (! m_db.tables().contains(QLatin1String(DB_LASTMESSAGE_TABLE))) {
        QSqlQuery query(m_db);
        if (!query.exec("create table last_user_message (id int primary key, userid text unique, last_message timestamp)")) {
            qDebug() << "create last_user_message table failed, query error=" << query.lastError();
            query.finish();
            return false;
        }
        query.finish();
    }
    return true;
}

/****************************************************************************************************************
 * User Last Message Table (last_user_message) handler functions
 ***************************************************************************************************************/

bool
WickrBotClientDatabase::updateLastUserMessage(const QString &userID) {
    if (!initialized)
        return false;
    bool retval = false;

    // Creation times are based on the client's time and date
    QString curTime = getClientTime();

    QSqlQuery query(m_db);
    QString queryString = QString("UPDATE last_user_message SET last_message='%1' where userid='%2'")
            .arg(curTime)
            .arg(userID);
    if (!query.exec(queryString)) {
        QSqlError error = query.lastError();
        qDebug() << "insertLastUserMessage: SQL error" << error;
        query.finish();
    } else {
        int numRows = query.numRowsAffected();
        query.finish();

        // If no rows are affected then insert the new record
        if (numRows == 0) {
            QSqlQuery query(m_db);
            QString queryString = QString("INSERT INTO last_user_message (userid, last_message) VALUES ('%1', '%2')")
                    .arg(userID)
                    .arg(curTime);
            if (!query.exec(queryString)) {
                QSqlError error = query.lastError();
                qDebug() << "insertLastUserMessage: SQL error" << error;
            } else {
                if (query.numRowsAffected() > 0)
                    retval = true;
            }
            query.finish();
        } else {
            retval = true;
        }
    }
    return retval;
}

QDateTime
WickrBotClientDatabase::getLastUserMessageTime(const QString &userID) {
    QDateTime retDate;

    if (!initialized)
        return retDate;

    QString queryString = "SELECT last_message FROM last_user_message WHERE userid = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, userID);

    if ( !query.exec()) {
        qDebug() << "getLastUserMessageTime: Could not retrieve" << userID;
    } else if (query.next()) {
        retDate = query.value(0).toDateTime();
    } else {
        qDebug() << "getLastUserMessageTime: No rows retrieved for" << userID;
    }
    query.finish();
    return retDate;
}

bool
WickrBotClientDatabase::deleteLastUserMessage(const QString &userID) {
    if (!initialized)
        return false;

    QString queryString = "DELETE FROM last_user_message WHERE userid = ?";
    QSqlQuery query(m_db);
    query.prepare(queryString);
    query.bindValue(0, userID);

    if ( !query.exec()) {
        qDebug() << "deleteLastUserMessage: Could not delete" << userID;
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}

bool
WickrBotClientDatabase::deleteLastUserOlderThan(const long seconds) {
    if (!initialized)
        return false;

    QString targetTime = getClientTimeWithOffset(seconds);

    QString queryString = QString("DELETE FROM last_user_message WHERE last_message < '%1'").arg(targetTime);
    QSqlQuery query(m_db);

    if ( !query.exec(queryString)) {
        qDebug() << "deleteLastUserOlderThan: Could not delete" << targetTime;
        qDebug() << "deleteLastUserOlderThan:" << query.lastError();
        query.finish();
        return false;
    }

    int numRows = query.numRowsAffected();
    query.finish();
    return (numRows > 0);
}
