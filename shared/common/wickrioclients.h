#ifndef WICKRIOCLIENTS_H
#define WICKRIOCLIENTS_H

#include "wickrbotlib.h"
#include "wickrbotclients.h"

class DECLSPEC WickrIOClients : public WickrBotClients
{
public:
    WickrIOClients() : WickrBotClients(), console_id(0)  {}
    WickrIOClients(const WickrBotClients &obj) : WickrBotClients(obj) {}

    ~WickrIOClients() {}

    QString getProcessName() { return(QString("%1.%2").arg(binary).arg(name)); }

public:
    int console_id;     // ID of the associated Console User or 0
};

#endif // WICKRIOCLIENTS_H

