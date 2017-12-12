#ifndef WICKRIORXSERVICE_H
#define WICKRIORXSERVICE_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickrIOServiceBase.h"

#include "operationdata.h"

#include "services/wickrMessageService.h"
#include "services/wickrSwitchboardService.h"

#include "messaging/wickrMessage.h"
#include "filetransfer/wickrFileInfo.h"
#include "wickrIOFileDownloadService.h"

// Forward declaration
class WickrIORxThread;
class WickrIORxDetails;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIORxService : public WickrIOServiceBase
{
    Q_OBJECT
public:
    explicit WickrIORxService(OperationData *operation, WickrIORxDetails *details);
    virtual ~WickrIORxService();

    void shutdown();
    bool isHealthy();

    void startReceive();
    void stopReceive();

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock  m_lock;
    WickrIORxThread         *m_rxThread;
    bool                    m_isHealthy = true;

    void startThreads(OperationData *operation, WickrIORxDetails *details);
    void stopThreads();

signals:
    void signalProcessStarted();
    void signalShutdown();
    void signalStartReceive();
    void signalStopReceive();
    void signalReceivingStarted();
    void signalReceivingEnded();

public slots:
    void slotIsHealthy(bool health);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


class WickrIORxThread : public QObject
{
    Q_OBJECT
public:
    WickrIORxThread(QThread *thread, WickrIORxService *svc, OperationData *operation, WickrIORxDetails *details);
    virtual ~WickrIORxThread();

private:
    OperationData *m_operation;
    bool m_enableSwitchboard = false;
    bool m_receiving = false;
    int m_timerStatsTicker = 0;

    bool                    m_running = false;
    QTimer                  m_timer;
    WickrIORxDetails   *m_details = nullptr;

    // Used to process incoming messages
    WickrConvoList *m_convoList = nullptr;

    // File Download definitions
    QMap<QString, WickrIORxDownloadFile *> m_activeDownloads;

    void startSwitchboard();
    void stopSwitchboard();
    void attachConvos();
    void detachConvos();

    void attachConvosMessages(WickrNotifyList *msgList);
    void detachConvosMessages(WickrNotifyList *msgList);
    bool processMessage(WickrDBObject *item);

public slots:
    void slotShutdown();
    void slotStartReceiving();
    void slotStopReceiving();
    void slotProcessStarted();

private slots:
    void slotProcessMessage(WickrDBObject *item);

    void slotConvoAdded(WickrDBObject *item, bool existing = false);
    void slotConvoChanged(WickrDBObject *item);
    void slotConvoDeleted(WickrDBObject *inItem);

    void slotOnTimerReceive();

signals:
    void signalNotRunning();
    void signalReceivingStarted();
    void signalReceivingEnded();
    void signalHealthUpdate(bool health);
    void signalProcessStarted();

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIORxDetails : public QObject
{
    Q_OBJECT
public:
    WickrIORxDetails(OperationData *);
    virtual ~WickrIORxDetails() {}

    virtual bool init() = 0;
    virtual bool processMessage(WickrDBObject *item) = 0;
    virtual bool healthCheck() = 0;

    void logCounts();
    void initCounts();

protected:
    OperationData *m_operation;
    int m_messagesRecv;
    int m_messagesRecvFailed;

};

#endif // WICKRIORXSERVICE_H
