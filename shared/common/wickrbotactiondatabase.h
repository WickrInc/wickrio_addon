#ifndef WICKRBOTACTIONDATABASE_H
#define WICKRBOTACTIONDATABASE_H

#include <QSqlRelationalTableModel>
#include <QString>

#include "wickrbotdatabase.h"
#include "wickrbotactioncache.h"

class WickrBotActionDatabase : public WickrBotDatabase
{
    Q_OBJECT

public:
    static bool getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action);
    static bool getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action, int clientID);

private:
    static QString getServerTime();

};

#endif // WICKRBOTACTIONDATABASE_H
