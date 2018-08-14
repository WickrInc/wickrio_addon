#ifndef CMDMAIN_H
#define CMDMAIN_H

#include "cmdbase.h"
#include "wickrbotclients.h"
#include "wickriodatabase.h"
#include "wickrIOIPCService.h"
#include "operationdata.h"
#include "cmdserver.h"

class CmdMain : public CmdBase
{
    Q_OBJECT
public:
    CmdMain(QCoreApplication *app, int argc, char **argv, OperationData *operation, WickrIOIPCService *ipcSvc);
    ~CmdMain();

    bool runCommands();

private:
    QCoreApplication    *m_app;
    int                 m_argc;
    char                **m_argv;

    QList<WickrBotClients *> m_clients;
    WickrIOIPCService       *m_ipcSvc;

    QString                 m_clientState;
    bool                    m_clientStateChanged;

    // Client Message handling values
    bool m_clientMsgSuccess;
    bool m_clientMsgInProcess;

    CmdOperation m_cmdOperation;
    CmdServer    m_cmdServer;

    bool updateBotList();
    bool validateIndex(int clientIndex);
    bool listBots();
    void resetClient(int clientIndex);
    void deleteClient(int clientIndex);
    void startClient(int clientIndex, bool force);
    void pauseClient(int clientIndex);
    void configClient(int clientIndex);

    bool sendClientCmd(const QString& name, const QString& cmd);
    bool sendClientServerCmd(const QString& cmd);

public slots:
    void signalReceivedMessage(QString type, QString value);

};

#endif // CMDMAIN_H
