#ifndef TESTCLIENTRXDETAILS_H
#define TESTCLIENTRXDETAILS_H

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

class TestClientRxDetails : public WickrIORxDetails
{
    Q_OBJECT
public:
    TestClientRxDetails(OperationData *operation);
    ~TestClientRxDetails();

    bool init();
    bool processMessage(WickrDBObject *item);
    bool healthCheck();

private:
    bool            m_receiving;
    bool            m_processing;

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

};

#endif // TESTCLIENTRXDETAILS_H
