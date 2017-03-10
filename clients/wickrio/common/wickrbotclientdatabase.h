#ifndef WICKRBOTCLIENTDATABASE_H
#define WICKRBOTCLIENTDATABASE_H

#include <QSqlRelationalTableModel>
#include <QString>

#include "wickrbotdatabase.h"
#include "wickrbotactioncache.h"
#include "wickrbotprocessstate.h"
#include "wickrbotclients.h"

class WickrBotClientDatabase : public WickrBotDatabase
{
    Q_OBJECT

public:
    WickrBotClientDatabase(const QString &dirPath);

    bool updateLastUserMessage(const QString &userID);
    QDateTime getLastUserMessageTime(const QString &userID);
    bool deleteLastUserMessage(const QString &userID);
    bool deleteLastUserOlderThan(const long seconds);

protected:
    bool createRelationalTables();

};

#define DB_LASTMESSAGE_TABLE    "last_user_message"

#endif // WICKRBOTCLIENTDATABASE_H
