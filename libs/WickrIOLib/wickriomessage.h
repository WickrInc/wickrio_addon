#ifndef WICKRIOMESSAGE_H
#define WICKRIOMESSAGE_H

#include <QString>
#include "wickrbotlib.h"

class WickrIOMessage //DECLSPEC
{
public:
    WickrIOMessage();

public:
    int id;
    int clientID;
    long timestamp;
    int type;
    QString json;
    bool hasAttachment;

};

#endif // WICKRIOMESSAGE_H
