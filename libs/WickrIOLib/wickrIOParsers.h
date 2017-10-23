#ifndef WICKRIOPARSERS_H
#define WICKRIOPARSERS_H

#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrIOParsers
{
public:
    WickrIOParsers() {}
    ~WickrIOParsers() {}

public:
    int     m_id;
    QString m_name;
    QString m_binary;
};

#endif // WICKRIOPARSERS_H

