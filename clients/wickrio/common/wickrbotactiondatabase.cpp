#include <QtSql>
#include <QDebug>
#include <QDateTime>
#include "wickrbotactiondatabase.h"
#include "wickrbotactioncache.h"

#include "session/wickrSession.h"

/**
 * @brief WickrBotActionDatabase::getFirstAction
 * This method will retrieve the first action in the database that has a runtime value less than
 * the current time. The input is a pointer to an WickrBotActionCache record to be populated with
 * the contents of the retrieved record.
 * @param action Pointer to the record to populate
 * @return true if a record is found, false if not
 */
bool
WickrBotActionDatabase::getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action) {
    // Actions from the server are based on the server's time
    QString dateTime = getServerTime();

    return botDB->getFirstAction(dateTime, action);
}

bool
WickrBotActionDatabase::getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action, int clientID) {
    // Actions from the server are based on the server's time
    QString dateTime = getServerTime();

    return botDB->getFirstAction(dateTime, action, clientID);
}

/**
 * @brief getServerTime
 * This function will return the string version of the Server's currently known date and time.
 * The format is based on the DB_DATETIME_FORAMT defined in the header file
 * @return Formatted string version of server's date and time
 */
QString WickrBotActionDatabase::getServerTime()
{
    long currentTime = WickrCore::WickrSession::getActiveSession()->getLocalAppClock()->getCurrentTime();
    QDateTime dt = QDateTime::fromTime_t((uint)currentTime);
    QDateTime dt2 = dt.toUTC();
    QString dateTime = dt2.toString(DB_DATETIME_FORMAT);
//    QString dateTime = dt.toString(DB_DATETIME_FORMAT);
    return dateTime;
}
