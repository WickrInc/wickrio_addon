#ifndef CMDPROVISIONING_H
#define CMDPROVISIONING_H

#include <QObject>
#include <QString>
#include "cmdbase.h"

class CmdProvisioning : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdProvisioning();

    bool runCommands();
    void status();

    QString m_clientType;       // Depends on what clients are installed

    QString m_configFileName;
    QString m_configPassword;

    QString m_username;
    QString m_regToken;
    QString m_invitation;

private:

};

#endif // CMDPROVISIONING_H

