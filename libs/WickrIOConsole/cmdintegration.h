#ifndef CMDINTEGRATION_H
#define CMDINTEGRATION_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "cmdbase.h"
#include "cmdoperation.h"
#include "cmdconsole.h"

#include <QProcess>

class CmdIntegration : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdIntegration(CmdOperation *operation);

    bool runCommands(QString commands=QString());
    void status();

private:
    CmdOperation            *m_operation;
    QList<WBIOBotTypes *>   m_customInts;

    bool processCommand(QStringList cmdList, bool &isquit);

    bool addIntegration(const QString& updateName = "");
    void deleteIntegration(int index);
    void listIntegrations();
    void updateIntegration(int index);

    bool validateIndex(int index);

    unsigned getVersionNumber(QFile *versionFile);
    void getVersionString(unsigned versionNum, QString& versionString);
    bool validateVersion(const QString& version);

    bool chkIntegrationExists(const QString& name);

};

#endif // CMDINTEGRATION_H
