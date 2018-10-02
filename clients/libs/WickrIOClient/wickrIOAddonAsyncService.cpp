#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>

#include "wickrIOAddonAsyncService.h"
#include "wickriodatabase.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOAPIInterface.h"
#include "wickrIOCommon.h"
#include "common/wickrUtil.h"

QString WickrIOAddonAsyncService::asyncServiceBaseName = "WickrIOAddonAsyncThread";

WickrIOAddonAsyncService::WickrIOAddonAsyncService() : WickrIOServiceBase(asyncServiceBaseName),
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_cbThread(nullptr)
{
  qRegisterMetaType<WickrServiceState>("WickrServiceState");
  qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

  // Start threads
  startThreads();

  setObjectName("WickrIOAddonAsyncThread");
  qDebug() << "WICKRIOJAVASCRIPT SERVICE: Started.";
  m_state = WickrServiceState::SERVICE_STARTED;
}

/**
* @brief WickrIOAddonAsyncService::~WickrIOAddonAsyncService
* Destructor
*/
WickrIOAddonAsyncService::~WickrIOAddonAsyncService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOJAVASCRIPT SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOAddonAsyncService::startThreads
 * Starts all threads on message service.
 */
void WickrIOAddonAsyncService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new WickrIOAddonAsyncThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOAddonAsyncService::signalStartScript,
            m_cbThread, &WickrIOAddonAsyncThread::slotStartScript, Qt::QueuedConnection);

    connect(m_cbThread, &WickrIOAddonAsyncThread::signalAsyncMessageSent, this, &WickrIOAddonAsyncService::signalAsyncMessageSent);
    connect(m_cbThread, &WickrIOAddonAsyncThread::signalAsyncEventSent, this, &WickrIOAddonAsyncService::signalAsyncEventSent);

    connect(this, &WickrIOAddonAsyncService::signalAsyncMessagesState,
            m_cbThread, &WickrIOAddonAsyncThread::slotAsyncMessagesState, Qt::QueuedConnection);
    connect(this, &WickrIOAddonAsyncService::signalAsyncEventsState,
            m_cbThread, &WickrIOAddonAsyncThread::slotAsyncEventsState, Qt::QueuedConnection);

    connect(this, &WickrIOAddonAsyncService::signalSendAsyncMessage,
            m_cbThread, &WickrIOAddonAsyncThread::slotSendAsyncMessage, Qt::QueuedConnection);
    connect(this, &WickrIOAddonAsyncService::signalSendAsyncEvent,
            m_cbThread, &WickrIOAddonAsyncThread::slotSendAsyncEvent, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOAddonAsyncService::stopThreads
 * Stops all threads on JavaScript service.
 */
void WickrIOAddonAsyncService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("JSCRIPT THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOAddonAsyncService::startScript()
{
    emit signalStartScript();
}

/**
 * @brief WickrIOAddonAsyncService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Event Handler this is typically related to a stuck event.
 * @return
 */
bool WickrIOAddonAsyncService::isHealthy()
{
    return true;
}

void WickrIOAddonAsyncService::setAsyncMessagesState(bool state)
{
    emit signalAsyncMessagesState(state);
}

bool WickrIOAddonAsyncService::asyncMessagesState()
{
    return (m_cbThread ? m_cbThread->asyncMessagesState() : false);
}

bool WickrIOAddonAsyncService::sendAsyncMessage(const QString& msg)
{
    emit signalSendAsyncMessage(msg);
}


void WickrIOAddonAsyncService::setAsyncEventsState(bool state)
{
    emit signalAsyncEventsState(state);
}

bool WickrIOAddonAsyncService::asyncEventsState()
{
    return (m_cbThread ? m_cbThread->asyncEventsState() : false);
}

bool WickrIOAddonAsyncService::sendAsyncEvent(const QString& event)
{
    emit signalSendAsyncEvent(event);
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOAddonAsyncThread::asStringState
 * @param state
 * @return
 */
QString WickrIOAddonAsyncThread::jsStringState(AddonAsyncThreadState state)
{
    switch(state) {
    case AddonAsyncThreadState::ADDONASYNC_UNINITIALIZED:   return "Uninitialized";
    case AddonAsyncThreadState::ADDONASYNC_STARTED:         return "Started";
    case AddonAsyncThreadState::ADDONASYNC_PROCESSING:      return "Processing";
    case AddonAsyncThreadState::ADDONASYNC_FINISHED:        return "Finished";
    default:                                return "Unknown";
    }
}

/**
 * @brief WickrIOAddonAsyncThread::WickrIOAddonAsyncThread
 * Constructor
 */
WickrIOAddonAsyncThread::WickrIOAddonAsyncThread(QThread *thread, WickrIOAddonAsyncService *cbSvc, QObject *parent)
    : QObject(parent),
      m_parent(cbSvc)
    , m_state(AddonAsyncThreadState::ADDONASYNC_UNINITIALIZED)
{
    thread->setObjectName("WickrIOAddonAsyncThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
    m_timer.start(1000);

    m_state = AddonAsyncThreadState::ADDONASYNC_STARTED;
}

/**
 * @brief WickrIOAddonAsyncThread::~WickrIOAddonAsyncThread
 * Destructor
 */
WickrIOAddonAsyncThread::~WickrIOAddonAsyncThread() {
    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));

    qDebug() << "WICKRIOJAVASCRIPT THREAD: Worker Destroyed.";
}

void
WickrIOAddonAsyncThread::slotTimerExpire()
{
    // If we are processing check if we are waiting for a response
    if (m_state == AddonAsyncThreadState::ADDONASYNC_PROCESSING) {
        if (m_asyncMesgSent) {
            time_t now;
            time(&now);
            // If have not received a response in over 5 seconds then fail the message
            if (now > (m_sentMessageTime + 5)) {
                qDebug() << "Timed out waiting for async message response!";
                emit signalAsyncMessageSent(false);
                m_failures++;

                //TODOShould reset the socket

                m_asyncMesgSent = false;
            }
        }
    }
}

void
WickrIOAddonAsyncThread::slotStartScript()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == AddonAsyncThreadState::ADDONASYNC_PROCESSING)
        return;

    m_zctx = nzmqt::createDefaultContext();

    m_async_zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REQ, this);
    m_async_zsocket->setObjectName("Requester.Socket.socket(REQ)");
    connect(m_async_zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &WickrIOAddonAsyncThread::slotAsyncResponseReceived, Qt::QueuedConnection);

    m_zctx->start();

    OperationData* operation = WickrIOClientRuntime::operationData();

    QString queueDirName = QString(WBIO_CLIENT_SOCKETDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
    QDir    queueDir(queueDirName);
    if (!queueDir.exists()) {
        if (!queueDir.mkpath(queueDirName)) {
            qDebug() << "Cannot create message queue directory!";
        }
    }

    // Create the socket file for the addon async messaging queue
    {
        QString queueName = QString(WBIO_ASYNC_TXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
        m_async_zsocket->bindTo(queueName);

        // Set the permission of the queue file so that normal user programs can access
        QString queueFileName = QString(WBIO_ASYNC_SOCKETFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
        QFile zmqFile(queueFileName);
        if(!zmqFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                   QFile::ReadUser | QFile::WriteUser | QFile::ExeUser  |
                                   QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                                   QFile::ReadOther | QFile::WriteOther | QFile::ExeOther)) {
            qDebug("Something wrong setting permissions of the async messaging queue file!");
        }
    }

    m_state = AddonAsyncThreadState::ADDONASYNC_PROCESSING;
}

/**********************************************************************************************
 * ASYNC MESSAGE HANDLING FUNCTIONS
 **********************************************************************************************/

void WickrIOAddonAsyncThread::slotAsyncMessagesState(bool state)
{
    m_processAsyncMessages = state;

    if (state) {
        WickrIOClientRuntime::cbSvcMessagesPending();
    }
}

void
WickrIOAddonAsyncThread::slotSendAsyncMessage(QString msg)
{
    // Not started yet or message was sent and waiting for a response
    if (m_state != AddonAsyncThreadState::ADDONASYNC_PROCESSING || m_asyncMesgSent) {
        qDebug() << "AsyncSend: Waiting for a response, can't send!";
        emit signalAsyncMessageSent(false);
        return;
    }

    QList<QByteArray> request;
    request += msg.toLocal8Bit();

    if (msg.length() > 0)
        qDebug() << "Sending async message:" << msg;

    if (!m_async_zsocket->sendMessage(request)) {
        qDebug() << "Failed to send async message!";
        emit signalAsyncMessageSent(false);
        m_failures++;
    } else {
        time(&m_sentMessageTime);   // Set the time that message is sent for timeout
        m_asyncMesgSent = true;
    }
}

void
WickrIOAddonAsyncThread::slotSendAsyncEvent(QString event)
{
    //TODO: Add actual code to send the message to the node.js addon
    emit signalAsyncEventSent(true);
}

void
WickrIOAddonAsyncThread::slotAsyncResponseReceived(const QList<QByteArray>& messages)
{
    for (QByteArray mesg : messages) {
        if (m_asyncMesgSent) {
            m_asyncMesgSent = false;
            qDebug() << "Got async message response:" << QString(mesg);
            emit signalAsyncMessageSent(QString(mesg) == "success");
        }
    }
}

void WickrIOAddonAsyncThread::slotAsyncEventsState(bool state)
{
    m_processAsyncEvents = state;
}
