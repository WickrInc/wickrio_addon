#ifndef WICKRIOBOT_H
#define WICKRIOBOT_H

#include <QCoreApplication>

#include "wickrioclients.h"
#include "cmdProvisioning.h"

class WickrIOBot
{
public:
    WickrIOBot(QCoreApplication *app, int argc, char **argv);

    bool newBotCreate();

private:
    QCoreApplication    *m_app;
    int                 m_argc;
    char                **m_argv;
    WickrIOClients      m_client;
    CmdProvisioning     m_provision;

    bool        m_dbEncrypt;

    QString     m_clientDbPath;
    QString     m_wbConfigFile;

};

#endif // WICKRIOBOT_H
