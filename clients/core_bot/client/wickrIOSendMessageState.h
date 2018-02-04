#ifndef WICKRIOSENDMESSAGESTATE_H
#define WICKRIOSENDMESSAGESTATE_H

#include <QDateTime>

#include "wickrIOCmdState.h"

class WickrIOSendMessageState : public WickrIOCmdState
{
public:
    WickrIOSendMessageState(const QString key) : WickrIOCmdState(key) {}
    WickrIOSendMessageState(const QString key, const QString originalCommand) : WickrIOCmdState(key, originalCommand) {}

    QString         m_message;
    QStringList     m_users;
    int             m_bor;
    bool            m_has_bor;
    int             m_ttl;
    bool            m_has_ttl;
    QDateTime       m_runTime;
};

#endif // WICKRIOSENDMESSAGESTATE_H
