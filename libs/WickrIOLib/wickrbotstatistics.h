#ifndef WICKRBOTSTATISTICS_H
#define WICKRBOTSTATISTICS_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotStatistics
{
public:
    WickrBotStatistics();

public:
    int id;
    int clientID;
    int statID;
    QString statDesc;
    qlonglong statValue;

public:
};

#endif // WICKRBOTSTATISTICS_H
