#ifndef WELCOMECLIENTRXDETAILS_H
#define WELCOMECLIENTRXDETAILS_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickrIORxService.h"
#include "operationdata.h"

#include "services/wickrMessageService.h"
#include "services/wickrSwitchboardService.h"

#include "messaging/wickrMessage.h"
#include "filetransfer/wickrFileInfo.h"
#include "wickrIOFileDownloadService.h"
#include "createjson.h"

class WelcomeClientRxDetails : public WickrIORxDetails
{
    Q_OBJECT
public:
    WelcomeClientRxDetails(OperationData *operation);
    ~WelcomeClientRxDetails();

    bool init();
    bool processMessage(WickrDBObject *item);
    bool healthCheck();

private:
    bool            m_receiving;
    bool            m_processing;

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

    // List of possible messages to respond with, when receiving messages
    QStringList         m_responseMessagesList;
    QMap<QString, int>  m_userResponseIndexMap;

    const QString& responseMessageText(const QString& userid);

};

#endif // WELCOMECLIENTRXDETAILS_H
