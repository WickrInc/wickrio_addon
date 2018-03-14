#ifndef WICKRIOCONSOLEPARSERHANDLER_H
#define WICKRIOCONSOLEPARSERHANDLER_H

#include <QString>

#include "wickriodatabase.h"
#include "wickrIOParsers.h"
#include "wickrbotprocessstate.h"


class WickrIOConsoleParserHandler
{

public:
    static QString addParser(WickrIOClientDatabase *ioDB, WickrIOParsers *newParser);
    static QString modifyParser(WickrIOClientDatabase *ioDB, WickrIOParsers *newParser);
    static QString handleSettings(WickrIOClientDatabase *ioDB, WickrIOParsers *newParser);
    static QStringList getNetworkInterfaceList();

    static QString getActualProcessState(const QString &processName, WickrIOClientDatabase* ioDB, int timeout=60);

};

#endif // WICKRIOCONSOLEPARSERHANDLER_H
