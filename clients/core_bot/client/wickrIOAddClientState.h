#ifndef WICKRIOADDCLIENTSTATE_H
#define WICKRIOADDCLIENTSTATE_H

#include <QProcess>

#include "wickrIOCmdState.h"

class WickrIOAddClientState : public WickrIOCmdState
{
public:
    WickrIOAddClientState(const QString key) : WickrIOCmdState(key) {}
    WickrIOAddClientState(const QString key, const QString originalCommand) : WickrIOCmdState(key, originalCommand) {}

    QProcess    m_process;
};

#endif // WICKRIOADDCLIENTSTATE_H
