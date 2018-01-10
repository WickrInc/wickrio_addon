#include <QUuid>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include "wickrIOEventService.h"
#include "Wickr/aes/AESHelper.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "wickrbotjsondata.h"

WickrIOEventService::WickrIOEventService(OperationData *operation) : WickrIOServiceBase("WickrIOEventThread"),
    m_lock(QReadWriteLock::Recursive),
    m_ahThread(nullptr)
{
    m_state = WickrServiceState::SERVICE_UNINITIALIZED;

    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads(operation);

    setObjectName("WickrIOEventThread");
    qDebug() << "WICKRIOEVENT SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIOEventService::~WickrIOEventService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOEVENT SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WIckrIOEventService::startThreads
 * Starts all threads on message service.
 */
void WickrIOEventService::startThreads(OperationData *operation)
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_ahThread = new WickrIOEventThread(&m_thread, this, operation);

    // connect to signals

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WIckrIOEventService::stopThreads
 * Stops all threads on event handler service.
 */
void WickrIOEventService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOEVENT THREAD: Shutdown Thread (%p)", &m_thread);
}

/**
 * @brief WIckrIOEventService::shutdown
 * Called to shutdown the event handler service.
 */
void WickrIOEventService::shutdown()
{
    QEventLoop wait_loop;
    connect(m_ahThread, &WickrIOEventThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdown();

    qDebug() << "Event Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "Event Service shutdown: finished waiting";
}

/**
 * @brief WickrIOEventService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Event Handler this is typically related to a stuck event.
 * @return
 */
bool WickrIOEventService::isHealthy()
{
    return true;
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


WickrIOEventThread::WickrIOEventThread(QThread *thread, WickrIOEventService *ahSvc, OperationData *operation) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(ahSvc),
    m_running(false),
    m_operation(operation)
{
    thread->setObjectName("WickrIOEventThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
    m_timer.start(1000 * WICKRIO_AH_UPDATE_PROCESS_SECS);

    initCounts();

    // Catch the shutdown signal
    connect(ahSvc, &WickrIOEventService::signalShutdown, this, &WickrIOEventThread::slotShutdown);

    // Catch the register and deregister signals

    m_running = true;
}

/**
 * @brief WickrIOEventThread::~WickrIOEventThread
 * Destructor
 */
WickrIOEventThread::~WickrIOEventThread() {
    m_running = false;

    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));

    qDebug() << "WICKRIOEVENT THREAD: Worker Destroyed.";
}

void
WickrIOEventThread::slotShutdown()
{
    m_running = false;
    if (m_timer.isActive())
        m_timer.stop();

    emit signalNotRunning();
}

/**
 * @brief WickrIOEventThread::doTimerWork
 * This method will perform operations that are to be done when our one second timer
 * goes off. The main signal is to initiate a Process Database operation.
 */
void WickrIOEventThread::slotTimerExpire()
{
    // If the process is not running then don't do anything
    if (!m_running) {
        return;
    }

    // Increment the App Counter, needed to backup the number of Apps sent to
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRIO_AH_UPDATE_STATS_SECS) == 0) {
    }

    // Process some of the events
    processEvents();
}

void
WickrIOEventThread::processEvents()
{
    QList<WickrIODBUser *> users;
    QList<WickrBotClients *> clients;
    QList<WickrBotClientEvents *> events;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return;
    }

    // Get all of the current events, up to 100 of them
    events = db->getClientEvents(100);

    // Get all of the console users
    users = db->getMotherUsers(m_operation->m_client->id);

    // Send the event to all users
    for (WickrIODBUser *user : users) {
        // Make sure the user wants to receive events
        if (!user->rxEvents()) {
            continue;
        }

        // Get the clients records.  If admin console user get all of the clients
        if (user->isAdmin()) {
            clients = db->getClients();
        } else {
            QList<int> clientIDs = db->getUserClients(user->m_id);
            for (int clientID : clientIDs) {
                WickrBotClients *client = db->getClient(clientID);
                clients.append(client);
            }
        }

        while (clients.length() > 0) {
            WickrBotClients * client = clients.first();
            clients.removeFirst();

            for (WickrBotClientEvents *event : events) {
                if (event->m_clientID == client->id) {
                    // Prepare a message to be sent
                    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);

                    QDateTime dt = QDateTime::currentDateTime();
                    jsonHandler->m_runTime = dt;
                    jsonHandler->m_vgroupid = QString("");
                    jsonHandler->m_userNames.append(user->m_user);
                    jsonHandler->m_userIDs.clear();
                    jsonHandler->m_message = QString("%1: %2")
                                                .arg(event->m_date.toString(DB_DATETIME_FORMAT))
                                                .arg(event->m_message);
                    jsonHandler->m_action = "sendmessage";
                    if (!jsonHandler->postEntry4SendMessage()) {
                        qDebug() << "Failed to send event!";
                    } else {
                        // Delete this event
                        event->m_deleteFlag = true;
                    }
                }
            }
        }
    }

    // Free the event memory
    for (WickrBotClientEvents *event : events) {
        // Delete this event
        if (event->m_deleteFlag && !event->m_isDeleted) {
            db->deleteClientEvent(event->id);
            event->m_isDeleted = true;
        }
        delete event;
    }
}
