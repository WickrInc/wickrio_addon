#ifndef WICKRBOTACTIONDATABASE_H
#define WICKRBOTACTIONDATABASE_H

#include "wickrbotdatabase.h"

class DECLSPEC WickrBotActionDatabase : public WickrBotDatabase
{
public:
    bool getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action);
    bool getFirstAction(WickrBotDatabase *botDB, WickrBotActionCache *action, int clientID);

private:
    QString getServerTime();

};

#endif // WICKRBOTACTIONDATABASE_H
