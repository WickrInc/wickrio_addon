#ifndef CORECLIENTRXDETAILS_H
#define CORECLIENTRXDETAILS_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickrIORxService.h"
#include "operationdata.h"
#include "wickriodatabase.h"
#include "wickrIOCmdState.h"
#include "wickrIOSendMessageState.h"

#include "services/wickrMessageService.h"
#include "services/wickrSwitchboardService.h"

#include "messaging/wickrMessage.h"
#include "filetransfer/wickrFileInfo.h"
#include "wickrIOFileDownloadService.h"

class CoreClientRxDetails : public WickrIORxDetails
{
    Q_OBJECT
public:
    CoreClientRxDetails(OperationData *operation);
    ~CoreClientRxDetails();

    bool init();
    bool processMessage(WickrDBObject *item);
    bool healthCheck();

private:
    bool                m_receiving;
    bool                m_processing;
    WickrIODBUser       m_user;
    QString             m_userid;

    // List of command state values
    QMap<QString, WickrIOCmdState *>    m_cmdState;

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

    typedef enum { CMD_CLIENTFILE_LOG, CMD_CLIENTFILE_OUTPUT } CoreClientFileType;

    QString getClientList();
    QString getClients();
    QString getClientFile(const QString& clientName, CoreClientFileType type);
    QString pauseClient(const QString& clientName);
    QString startClient(const QString& clientName);
    QString getStats(const QString& clientName);

    QString sendFile(const QString& txt, WickrIOCmdState *cmdState);
    QString sendMessage(const QString& txt, WickrIOCmdState *cmdState);

    QString getIntegrationsList();


    WickrIOCmdState *getCmdState(const QString& userid);
    bool addCmdState(const QString& userid, WickrIOCmdState *state);
    bool deleteCmdState(const QString& userid);

    bool updateAndValidateMembers(const QStringList& memberslist, QString& errorString);
    bool postMessage(WickrIOSendMessageState *sendState);

signals:
    void signalMemberSearchDone();
};

#endif // CORECLIENTRXDETAILS_H
