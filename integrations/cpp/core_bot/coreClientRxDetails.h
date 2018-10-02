#ifndef CORECLIENTRXDETAILS_H
#define CORECLIENTRXDETAILS_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "coreOperationData.h"
#include "wickriodatabase.h"
#include "wickrIOCmdState.h"

class CoreClientRxDetails : public QObject
{
    Q_OBJECT
public:
    CoreClientRxDetails(CoreOperationData *operation);
    ~CoreClientRxDetails();

    bool processMessage(const QString& txt);

private:
    CoreOperationData   *m_operation;

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
    QString getStats(const QString& clientName);

    WickrIOCmdState *getCmdState(const QString& userid);
    bool addCmdState(const QString& userid, WickrIOCmdState *state);
    bool deleteCmdState(const QString& userid);
};

#endif // CORECLIENTRXDETAILS_H
