#ifndef CMDMAIN_H
#define CMDMAIN_H

#include "cmdbase.h"
#include "wickrioclients.h"
#include "wickriodatabase.h"
#include "wickrbotipc.h"
#include "wickrioipc.h"
#include "operationdata.h"

class CmdMain : public CmdBase
{
    Q_OBJECT
public:
    CmdMain(QCoreApplication *app, int argc, char **argv, OperationData *operation);
    ~CmdMain();

    bool runCommands();

private:
    QCoreApplication    *m_app;
    int                 m_argc;
    char                **m_argv;

    QList<WickrIOClients *> m_clients;
    WickrBotIPC             m_txIPC;
    WickrBotMainIPC         *m_rxIPC;

    OperationData           *m_operation;

    WickrIOClientDatabase   *m_ioDB;

    QString                 m_clientState;
    bool                    m_clientStateChanged;

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

public slots:
    void signalReceivedMessage(QString type, QString value);

};

#endif // CMDMAIN_H
