#ifndef WICKRIOCONSOLECLIENTHANDLER_H
#define WICKRIOCONSOLECLIENTHANDLER_H

#include <QString>

#include "wickriodatabase.h"
#include "wickrioclients.h"

class WickrIOConsoleClientHandler
{

public:
    static QString addClient(WickrIOClientDatabase *ioDB, WickrIOClients *newClient);
    static QStringList getNetworkInterfaceList();

    static bool validateSSLKey(const QString &sslKeyFile);
    static bool validateSSLCert(const QString &sslCertFile);

    static QString getActualProcessState(const QString &processName, WickrIOClientDatabase* ioDB, int timeout=60);

};

#endif // WICKRIOCONSOLECLIENTHANDLER_H
