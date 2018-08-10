#ifndef CMDPROVISIONING_H
#define CMDPROVISIONING_H

#include <QObject>
#include <QString>
#include "cmdbase.h"
#include "wickrbotclients.h"

class CmdProvisioning : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdProvisioning(WickrBotClients *client);

    bool runCommands(int argc, char *argv[]);
    void status();

    WickrBotClients  *m_client;

    QString m_configFileName;
    QString m_configPassword;

    QString m_invitation;

private:

};

#endif // CMDPROVISIONING_H

