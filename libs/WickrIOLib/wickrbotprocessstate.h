#ifndef WICKRBOTPROCESSSTATE_H
#define WICKRBOTPROCESSSTATE_H

#include <QDateTime>
#include <QString>
#include "wickrbotlib.h"

class DECLSPEC WickrBotProcessState
{
public:
    WickrBotProcessState();
    ~WickrBotProcessState();

public:
    int id;

    QString process;
    int process_id;
    int state;
    QDateTime last_update;

    int ipc_port;
};

#define PROCSTATE_PAUSED    2
#define PROCSTATE_RUNNING   1
#define PROCSTATE_DOWN      0

#endif // WICKRBOTPROCESSSTATE_H
