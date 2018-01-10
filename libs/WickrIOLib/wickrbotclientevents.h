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

    // Set during processing of an event
    bool        m_deleteFlag=false;     // Set when event should be deleted
    bool        m_isDeleted=false;      // Set when event has been deleted

    typedef enum { CriticalEvent, NormalEvent } WickrIOEventType;

    WickrIOEventType	m_type;
};

#endif
