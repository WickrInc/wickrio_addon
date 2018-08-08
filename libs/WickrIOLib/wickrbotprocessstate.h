#ifndef WICKRBOTPROCESSSTATE_H
#define WICKRBOTPROCESSSTATE_H

#include <QDateTime>
#include <QString>
#include <QObject>
#include "wickrbotlib.h"

class DECLSPEC WickrBotProcessState : public QObject
{
    Q_OBJECT
public:
    WickrBotProcessState();
    ~WickrBotProcessState();

public:
    int id;

    QString process;
    int process_id;
    int state;
    QDateTime last_update;
};

#define PROCSTATE_RESET     100     // Client is to be reset, action as opposed to a state
#define PROCSTATE_PAUSED    2       // Client is paused, or pause a client action
#define PROCSTATE_RUNNING   1       // Client is running
#define PROCSTATE_DOWN      0       // Client is down, initial state to start a client
#define PROCSTATE_UNKNOWN   -99     // Do not know the state

#endif // WICKRBOTPROCESSSTATE_H
