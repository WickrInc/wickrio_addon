#ifndef CMDMAIN_H
#define CMDMAIN_H

#include "cmdbase.h"
#include "wickrioclients.h"
#include "wickriodatabase.h"
#include "wickrbotipc.h"

class CmdMain : public CmdBase
{
    Q_OBJECT
public:
    CmdMain(QCoreApplication *app, int argc, char **argv);
    ~CmdMain();

    bool runCommands();

private:
    QCoreApplication    *m_app;
    int                 m_argc;
    char                **m_argv;

    WickrIOClientDatabase   *m_ioDB;
    QList<WickrIOClients *> m_clients;
    WickrBotIPC             m_ipc;

    // Client Message handling values
    bool m_clientMsgSuccess;
    bool m_clientMsgInProcess;

    bool updateBotList();
    bool validateIndex(int clientIndex);
    bool listBots();
    void resetClient(int clientIndex);
    void deleteClient(int clientIndex);
    void startClient(int clientIndex, bool force);
    void pauseClient(int clientIndex);
    bool sendClientCmd(int port, const QString& cmd);
    void configClient(int clientIndex);

};

#endif // CMDMAIN_H
