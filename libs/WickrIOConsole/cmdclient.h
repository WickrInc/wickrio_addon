#ifndef CMDCLIENT_H
#define CMDCLIENT_H

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

class CmdClient : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdClient(CmdOperation *operation);

    bool runCommands(QString commands=QString());
    void status();

private:
    bool processCommand(QStringList cmdList, bool &isquit);

    bool getClientValues(WickrBotClients *client);
    void addClient();
    void deleteClient(int clientIndex);
    void listClients();
    void modifyClient(int clientIndex);
    void pauseClient(int clientIndex, bool force);
    void startClient(int clientIndex, bool force);

    bool chkClientsNameExists(const QString& name);
    bool chkClientsUserExists(const QString& name);
    bool chkClientsInterfaceExists(const QString& iface, int port);

    bool validateIndex(int clientIndex);

    bool sendClientCmd(int port, const QString& cmd);

    bool runBotScript(const QString& destPath, const QString& configure, WickrBotClients *client);

    bool getAuthValue(WickrBotClients *client, bool basic, QString& authValue);

private:
    CmdOperation *m_operation;
    WickrIOSSLSettings m_sslSettings;

    // Client Message handling values
    bool m_clientMsgSuccess;
    bool m_clientMsgInProcess;

    QList<WickrBotClients *> m_clients;

    QProcess *m_exec;


private slots:
    void slotCmdFinished(int, QProcess::ExitStatus);
    void slotCmdOutputRx();

};

#endif // CMDCLIENT_H
