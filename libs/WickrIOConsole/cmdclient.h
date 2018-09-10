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
#include "cmdintegration.h"

#include <QProcess>

class CmdClient : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdClient(CmdOperation *operation);

    bool runCommands(const QStringList& options, QString commands=QString());
    void status();
    void setBasicConfig(bool basicConfig) { m_basicConfig = basicConfig; }

private:
    bool processCommand(QStringList cmdList, bool &isquit);
    void processHelp(const QStringList& cmdList);

    bool getClientValues(WickrBotClients *client, bool fromConfig=false);
    void addClient();
    void deleteClient(int clientIndex);
    void listClients();
    void listInboundPorts();
    void modifyClient(int clientIndex);
    void pauseClient(int clientIndex, bool force);
    void startClient(int clientIndex, bool force);
    void upgradeClient(int clientIndex);

    bool chkClientsUserExists(const QString& name);
    bool chkClientsInterfaceExists(const QString& iface, int port);

    bool validateIndex(int clientIndex);

    bool sendIPCCmd(const QString& dest, bool isClient, const QString& cmd);
    void closeClientIPC(const QString& dest);

    bool readLineFromProcess(QProcess *process, QString& line);
    bool runBotScript(const QString& destPath, const QString& configure, WickrBotClients *client, const QStringList& args);

    bool getAuthValue(WickrBotClients *client, bool basic, QString& authValue);

    unsigned getVersionNumber(QFile *versionFile);
    void getIntegrationVersionString(unsigned versionNum, QString& versionString);
    void integrationUpdateVersionFile(const QString& path, const QString& version);

    // Integration bot commands
    bool integrationCopySW(WickrBotClients *client, const QString& swPath, const QString& destPath);
    bool integrationInstall(WickrBotClients *client, const QString& destPath);
    bool integrationConfigure(WickrBotClients *client, const QString& destPath);
    bool integrationUpgrade(WickrBotClients *client, const QString& curSWPath, const QString& newSWPath);

    bool configClients();

private:
    CmdOperation        *m_operation;
    CmdIntegration      m_cmdIntegration;
    WickrIOSSLSettings  m_sslSettings;

    // Client Message handling values
    bool    m_clientMsgSuccess;
    bool    m_clientMsgInProcess;

    bool    m_basicConfig = false;
    bool    m_root = false;

    QList<WickrBotClients *> m_clients;
    QMap<QString, unsigned>  m_integrationVersions;

    QProcess *m_exec;

    bool                    m_clientStateChanged = false;
    QString                 m_clientState;

    void updateIntegrationVersion();

private slots:
    void slotCmdFinished(int);
    void slotCmdOutputRx();

    void slotReceivedMessage(QString type, QString value);

};

#endif // CMDCLIENT_H
