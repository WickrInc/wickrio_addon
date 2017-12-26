#ifndef WICKRBOTCLIENTEVENTS_H
#define WICKRBOTCLIENTEVENTS_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotClientEvents
{
public:
    WickrBotClientEvents() {};
    ~WickrBotClientEvents() {};

public:
    int         id=0;
    int         m_clientID=0;
    QString     m_message;
    QDateTime   m_date;

    typedef enum { CriticalEvent, NormalEvent } WickrIOEventType;

    WickrIOEventType	m_type;
};

#endif
