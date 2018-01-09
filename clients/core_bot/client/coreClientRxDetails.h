#ifndef CORECLIENTRXDETAILS_H
#define CORECLIENTRXDETAILS_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickrIORxService.h"
#include "operationdata.h"
#include "wickriodatabase.h"

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

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

    typedef enum { CMD_CLIENTFILE_LOG, CMD_CLIENTFILE_OUTPUT } CoreClientFileType;

    QString getClientList();
    QString getClients();
    QString getClientFile(const QString& clientName, CoreClientFileType type);
    QString pauseClient(const QString& clientName);
    QString startClient(const QString& clientName);

};

#endif // CORECLIENTRXDETAILS_H
