#ifndef WICKRIOBOT_H
#define WICKRIOBOT_H

#include <QCoreApplication>

#include "wickrbotclients.h"
#include "cmdProvisioning.h"
#include "wickriodatabase.h"

class WickrIOBot
{
public:
    WickrIOBot(QCoreApplication *app, int argc, char **argv, WickrIOClientDatabase *ioDB);
    WickrIOBot(QCoreApplication *app, WickrBotClients *client, WickrIOClientDatabase *ioDB);

    bool newBotCreate();
    bool botUpdate();

private:
    QCoreApplication        *m_app;
    int                     m_argc;
    char                    **m_argv;
    WickrBotClients         m_client;
    CmdProvisioning         m_provision;
    WickrIOClientDatabase   *m_ioDB;

    bool        m_dbEncrypt;

    QString     m_clientDbPath;
    QString     m_wbConfigFile;

    bool        m_loginDone;
};

#endif // WICKRIOBOT_H
