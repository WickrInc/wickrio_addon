#ifndef CMDCLIENT_H
#define CMDCLIENT_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrioappsettings.h"
#include "cmdbase.h"
#include "cmdoperation.h"
#include "cmdconsole.h"

#include <QProcess>

class CmdClient : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdClient(CmdOperation *operation);

    bool runCommands();
    void status();

private:
    bool getClientValues(WickrIOClients *client);
    void addClient();
    void deleteClient(int clientIndex);
    void listClients();
    void modifyClient(int clientIndex);
    void pauseClient(int clientIndex);
    void startClient(int clientIndex);

    bool chkClientsNameExists(const QString& name);
    bool chkClientsUserExists(const QString& name);
    bool chkClientsInterfaceExists(const QString& iface, int port);

    bool validateIndex(int clientIndex);

    bool sendClientCmd(int port, const QString& cmd);

private:
    CmdOperation *m_operation;
    WickrIOSSLSettings m_sslSettings;

    // Client Message handling values
    bool m_clientMsgSuccess;
    bool m_clientMsgInProcess;

    QList<WickrIOClients *> m_clients;

    QProcess *m_exec;


private slots:
    void slotCmdFinished(int, QProcess::ExitStatus);
    void slotCmdOutputRx();

};

#endif // CMDCLIENT_H
