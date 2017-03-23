#ifndef WICKRBOTACTIONCACHE_H
#define WICKRBOTACTIONCACHE_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotActionCache
{
public:
    WickrBotActionCache() {}
    ~WickrBotActionCache() {}

public:
    int id;
    QByteArray json;

    QDateTime created;
    QDateTime runTime;
    int attempts;
};

#endif // WICKRBOTACTIONCACHE_H

