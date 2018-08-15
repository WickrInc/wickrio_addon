#include <QJsonArray>
#include <QJsonDocument>

#include "wickrIORxService.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"

#include "common/wickrNotifyList.h"

#include "wickrIOClientRuntime.h"

#define UPDATE_STATS_SECS      3600

WickrIORxService::WickrIORxService(OperationData *operation, WickrIORxDetails *details) : WickrIOServiceBase("WickrIORxThread"),
    m_lock(QReadWriteLock::Recursive),
    m_rxThread(nullptr)
{
    m_state = WickrServiceState::SERVICE_UNINITIALIZED;

    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads(operation, details);

    setObjectName("WickrIORxThread");
    qDebug() << "WICKRIORECEIVE SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIORxService::~WickrIORxService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIORECEIVE SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIORxService::startThreads
 * Starts all threads on message service.
 */
void WickrIORxService::startThreads(OperationData *operation, WickrIORxDetails *details)
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_rxThread = new WickrIORxThread(&m_thread, this, operation, details);

    // connect to signals
    connect(m_rxThread, &WickrIORxThread::signalHealthUpdate, this, &WickrIORxService::slotIsHealthy);
    connect(m_rxThread, &WickrIORxThread::signalProcessStarted, this, &WickrIORxService::signalProcessStarted);
    connect(m_rxThread, &WickrIORxThread::signalReceivingStarted, this, &WickrIORxService::signalReceivingStarted);
    connect(m_rxThread, &WickrIORxThread::signalReceivingEnded, this, &WickrIORxService::signalReceivingEnded);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIORxService::stopThreads
 * Stops all threads on Receive handler service.
 */
void WickrIORxService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIORECEIVE THREAD: Shutdown Thread (%p)", &m_thread);
}

/**
 * @brief WickrIORxService::shutdown
 * Called to shutdown the Receive handler service.
 */
void WickrIORxService::shutdown()
{
    QEventLoop wait_loop;
    connect(m_rxThread, &WickrIORxThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdown();

    qDebug() << "Receive Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "Receive Service shutdown: finished waiting";
}

/**
 * @brief WickrIORxService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Receive Handler this is typically related to a stuck Receive.
 * @return
 */
bool WickrIORxService::isHealthy()
{
    return m_isHealthy;
}

/**
 * @brief WickrIORxService::slotIsHealthy
 * Update the current health of the receive service
 * @param health
 * @return
 */
void WickrIORxService::slotIsHealthy(bool health)
{
    m_isHealthy = health;
}

void WickrIORxService::startReceive()
{
    m_isReceiving = true;
    emit signalStartReceive();
}

void WickrIORxService::stopReceive()
{
    if (m_isReceiving) {
        m_isReceiving = false;
        emit signalStopReceive();
    }
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


WickrIORxThread::WickrIORxThread(QThread *thread, WickrIORxService *rxSvc, OperationData *operation, WickrIORxDetails *details) :
    m_operation(operation),
    m_details(details)
{
    thread->setObjectName("WickrIORxThread");
    this->moveToThread(thread);

    // Call the details init function
    if (m_details != nullptr) {
        m_details->init();
        m_details->initCounts();
    }

    // Signal to start the thread
    connect(thread, &QThread::started, this, &WickrIORxThread::slotProcessStarted);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotOnTimerReceive()));
    m_timer.start(1000);

    // Catch the shutdown signal
    connect(rxSvc, &WickrIORxService::signalShutdown, this, &WickrIORxThread::slotShutdown);

    // Catch the register and deregister signals
    connect(rxSvc, &WickrIORxService::signalStartReceive, this, &WickrIORxThread::slotStartReceiving);
    connect(rxSvc, &WickrIORxService::signalStopReceive, this, &WickrIORxThread::slotStopReceiving);

    m_running = true;
}

WickrIORxThread::~WickrIORxThread()
{
    slotShutdown();
    qDebug() << "WICKRIORECEIVE THREAD: Worker Destroyed.";
}

void WickrIORxThread::slotProcessStarted()
{
    qDebug() << "Started WickrIORxThread";

    // Login successful, so login to switchboard
    startSwitchboard();

    emit signalProcessStarted();
}

void WickrIORxThread::slotShutdown()
{
    m_running = false;
    slotStopReceiving();

    if (m_convoList != NULL) {
        slotStopReceiving();
    }

    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotOnTimerReceive()));

    emit signalNotRunning();
}


void WickrIORxThread::slotOnTimerReceive()
{
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % UPDATE_STATS_SECS) == 0 && m_details != nullptr) {
        m_details->logCounts();
    }
}

void WickrIORxThread::slotStartReceiving()
{
    if (! m_receiving) {
        m_receiving = true;

        m_convoList = WickrCore::WickrConvo::getConvoList();
        attachConvos();
    }
    emit signalReceivingStarted();
}

void WickrIORxThread::slotStopReceiving()
{
    if (m_receiving) {
        if (m_convoList != NULL) {
            detachConvos();
            m_convoList = NULL;
        }
        m_receiving = false;
    }
    emit signalReceivingEnded();
}




void
WickrIORxThread::slotProcessMessage(WickrDBObject *item)
{
    if (processMessage(item)) {
        WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;
        if (msg) {
            msg->dodelete(traceInfo());
//            delete msg;
        }
    }
}

bool
WickrIORxThread::processMessage(WickrDBObject *item)
{
    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return false;
    }

    if (m_details != nullptr) {
        return m_details->processMessage(item);
    }
    return false;
}

void WickrIORxThread::attachConvos()
{
    if (m_convoList) {
        QList<WickrDBObject *>items = m_convoList->acquireItemList(false);
        foreach(WickrDBObject *item, items) {
            slotConvoAdded(item, false);
        }

        connect(m_convoList, SIGNAL(addedItem(WickrDBObject*)),   this, SLOT(slotConvoAdded(WickrDBObject*)), Qt::QueuedConnection);
        connect(m_convoList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotConvoChanged(WickrDBObject*)), Qt::QueuedConnection);
        connect(m_convoList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoDeleted(WickrDBObject*)), Qt::QueuedConnection);

        m_convoList->releaseAcquire();
    }
}

void WickrIORxThread::detachConvos()
{
    if(m_convoList) {
        disconnect(m_convoList, SIGNAL(addedItem(WickrDBObject*)),   this, SLOT(slotConvoAdded(WickrDBObject*)));
        disconnect(m_convoList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotConvoChanged(WickrDBObject*)));
        disconnect(m_convoList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoDeleted(WickrDBObject*)));
    }
}

void WickrIORxThread::slotConvoAdded(WickrDBObject *item, bool existing)
{
    WickrCore::WickrConvo *convo = static_cast<WickrCore::WickrConvo *>(item);
    if (convo) {
        // TODO: Need to check if this convo is setup already or not
        if (existing) {
            // Check if the convo is in a Key Validation state that needs to be acted on
            if (convo->getKeyVerState() == WickrCore::WickrConvo::KeyVerStateGotVideo) {
                // We have the users video, so verify and send a key verification response
                WickrCore::WickrKeyVerificationMgr *wkvm = WickrCore::WickrRuntime::getKeyVerifyMgr();
                if (wkvm) {
                    wkvm->acceptVerification(convo);
                }
            }
        } else {
            attachConvosMessages(convo->getMessages());
        }
    }
}

void WickrIORxThread::slotConvoChanged(WickrDBObject *item) {
    slotConvoAdded(item, true);
}

void WickrIORxThread::slotConvoDeleted(WickrDBObject *inItem) {
    if(inItem != NULL) {
        WickrCore::WickrConvo *convo = static_cast<WickrCore::WickrConvo *>(inItem);
        detachConvosMessages(convo->getMessages());
    }
}


void WickrIORxThread::attachConvosMessages(WickrNotifyList *msgList)
{
    if (msgList != NULL) {
        QList<WickrDBObject*> toBeDeleted;
        QList<WickrDBObject*> items = msgList->acquireItemList(false);
        foreach(WickrDBObject *item, items) {
            //qDebug() << "**** FILTER " << item;
            if (item != NULL) {
                if (processMessage(item)) {
                    toBeDeleted.append(item);
                }
            }
        }

        connect(msgList, SIGNAL(addedItem  (WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
        connect(msgList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
//        connect(msgList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoMessageDeleted(WickrDBObject*)));

        msgList->releaseAcquire();

        foreach(WickrDBObject *item, toBeDeleted) {
            WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;
            if (msg) {
                msg->dodelete(traceInfo());
    //            delete msg;
            }
        }
    }
}

void WickrIORxThread::detachConvosMessages(WickrNotifyList *msgList)
{
    disconnect(msgList, SIGNAL(addedItem  (WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
    disconnect(msgList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
//    disconnect(msgList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoMessageDeleted(WickrDBObject*)));
}






void WickrIORxThread::startSwitchboard()
{
    // Update switchboard login credentials (login is performed only if not already logged in)
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin());
}

void WickrIORxThread::stopSwitchboard()
{
    WickrCore::WickrRuntime::swbSvcLogout();
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIORxDetails::WickrIORxDetails(OperationData *operation) :
    m_operation(operation)
{
}

void WickrIORxDetails::initCounts()
{
    m_messagesRecv = 0;
    m_messagesRecvFailed = 0;
    m_messagesRecvInvalid = 0;
}

void WickrIORxDetails::logCounts()
{
    int msgs = m_messagesRecv;            m_messagesRecv = 0;
    int fails = m_messagesRecvFailed;     m_messagesRecvFailed = 0;
    int invalids = m_messagesRecvInvalid; m_messagesRecvInvalid = 0;

    QString statsMsg = QString("Rx Statistics:\n  Messages received %1\n  Messages failed %2\n  Messages invalid %3\n")
            .arg(msgs).arg(fails).arg(invalids);
    m_operation->log_handler->log(statsMsg);
}
