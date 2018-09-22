#ifndef CORECLIENTRXDETAILS_H
#define CORECLIENTRXDETAILS_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "operationdata.h"
#include "wickriodatabase.h"
#include "wickrIOCmdState.h"

#include "messaging/wickrMessage.h"

class CoreClientRxDetails
{
    Q_OBJECT
public:
    CoreClientRxDetails(OperationData *operation);
    ~CoreClientRxDetails();

    bool processMessage(const QString& txt);

private:
    OperationData       *m_operation;

    bool                m_receiving;
    bool                m_processing;
    WickrIODBUser       m_user;
    QString             m_userid;

    // List of command state values
    QMap<QString, WickrIOCmdState *>    m_cmdState;

    typedef enum { CMD_CLIENTFILE_LOG, CMD_CLIENTFILE_OUTPUT } CoreClientFileType;

    QString getClientList();
    QString getClients();
    QString getClientFile(const QString& clientName, CoreClientFileType type);
    QString pauseClient(const QString& clientName);
    QString startClient(const QString& clientName);
    QString getStats(const QString& clientName);
    QString addClient(const QString& txt, WickrIOCmdState *cmdState);

    QString getIntegrationsList();


    WickrIOCmdState *getCmdState(const QString& userid);
    bool addCmdState(const QString& userid, WickrIOCmdState *state);
    bool deleteCmdState(const QString& userid);

signals:
    void signalMemberSearchDone();
};

#endif // CORECLIENTRXDETAILS_H
