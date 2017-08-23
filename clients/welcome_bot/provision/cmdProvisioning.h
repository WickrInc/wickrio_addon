#ifndef CMDPROVISIONING_H
#define CMDPROVISIONING_H

#include <QObject>
#include <QString>
#include "cmdbase.h"
#include "wickrioclients.h"

class CmdProvisioning : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdProvisioning(WickrIOClients *client);

    bool runCommands();
    void status();

    WickrIOClients  *m_client;

    QString m_configFileName;
    QString m_configPassword;

    QString m_invitation;

private:

};

#endif // CMDPROVISIONING_H

