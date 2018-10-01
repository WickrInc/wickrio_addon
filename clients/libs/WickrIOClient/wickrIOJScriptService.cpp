#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>

#include "wickrIOJScriptService.h"
#include "wickriodatabase.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOAPIInterface.h"
#include "wickrIOCommon.h"
#include "common/wickrUtil.h"
#include "wickrIOAddonAsyncService.h"

QString WickrIOJScriptService::jsServiceBaseName = "WickrIOJScriptThread";

WickrIOJScriptService::WickrIOJScriptService() : WickrIOServiceBase(jsServiceBaseName),
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_cbThread(nullptr)
{
  qRegisterMetaType<WickrServiceState>("WickrServiceState");
  qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

  // Start threads
  startThreads();

  setObjectName("WickrIOJScriptThread");
  qDebug() << "WICKRIOJAVASCRIPT SERVICE: Started.";
  m_state = WickrServiceState::SERVICE_STARTED;
}

/**
* @brief WickrIOJScriptService::~WickrIOJScriptService
* Destructor
*/
WickrIOJScriptService::~WickrIOJScriptService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOJAVASCRIPT SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOJScriptService::startThreads
 * Starts all threads on message service.
 */
void WickrIOJScriptService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new WickrIOJScriptThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOJScriptService::signalStartScript,
            m_cbThread, &WickrIOJScriptThread::slotStartScript, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOJScriptService::stopThreads
 * Stops all threads on JavaScript service.
 */
void WickrIOJScriptService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("JSCRIPT THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOJScriptService::startScript()
{
    emit signalStartScript();
}

/**
 * @brief WickrIOJScriptService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Event Handler this is typically related to a stuck event.
 * @return
 */
bool WickrIOJScriptService::isHealthy()
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

/**
 * @brief WickrIOJScriptThread::asStringState
 * @param state
 * @return
 */
QString WickrIOJScriptThread::jsStringState(JSThreadState state)
{
    switch(state) {
    case JSThreadState::JS_UNINITIALIZED:   return "Uninitialized";
    case JSThreadState::JS_STARTED:         return "Started";
    case JSThreadState::JS_PROCESSING:      return "Processing";
    case JSThreadState::JS_FINISHED:        return "Finished";
    default:                                return "Unknown";
    }
}

/**
 * @brief WickrIOJScriptThread::WickrIOJScriptThread
 * Constructor
 */
WickrIOJScriptThread::WickrIOJScriptThread(QThread *thread, WickrIOJScriptService *cbSvc, QObject *parent)
    : QObject(parent),
      m_parent(cbSvc)
    , m_state(JSThreadState::JS_UNINITIALIZED)
{
    thread->setObjectName("WickrIOJScriptThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
    m_timer.start(1000);

    m_state = JSThreadState::JS_STARTED;
}

/**
 * @brief WickrIOJScriptThread::~WickrIOJScriptThread
 * Destructor
 */
WickrIOJScriptThread::~WickrIOJScriptThread() {
    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));

    qDebug() << "WICKRIOJAVASCRIPT THREAD: Worker Destroyed.";
}

void
WickrIOJScriptThread::slotTimerExpire()
{
    // If we are processing check if we are waiting for a response
    if (m_state == JSThreadState::JS_PROCESSING) {
    }
}

void
WickrIOJScriptThread::slotStartScript()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;
    m_state = JSThreadState::JS_PROCESSING;

    m_zctx = nzmqt::createDefaultContext();

    m_zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REP, this);
    m_zsocket->setObjectName("Replier.Socket.socket(REP)");
    connect(m_zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &WickrIOJScriptThread::slotMessageReceived, Qt::QueuedConnection);

    m_zctx->start();

    OperationData* operation = WickrIOClientRuntime::operationData();

    QString queueDirName = QString(WBIO_CLIENT_SOCKETDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
    QDir    queueDir(queueDirName);
    if (!queueDir.exists()) {
        if (!queueDir.mkpath(queueDirName)) {
            qDebug() << "Cannot create message queue directory!";
        }
    }

    // Create the socket file for the addon receive queue
    {
        QString queueName = QString(WBIO_CLIENT_RXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
        m_zsocket->bindTo(queueName);

        // Set the permission of the queue file so that normal user programs can access
        QString queueFileName = QString(WBIO_CLIENT_SOCKETFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(operation->m_client->name);
        QFile zmqFile(queueFileName);
        if(!zmqFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                   QFile::ReadUser | QFile::WriteUser | QFile::ExeUser  |
                                   QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                                   QFile::ReadOther | QFile::WriteOther | QFile::ExeOther)) {
            qDebug("Something wrong setting permissions of the queue file!");
        }
    }

    m_state = JSThreadState::JS_STARTED;
}

/**********************************************************************************************
 * INCOMING MESSAGE HANDLING FUNCTIONS
 **********************************************************************************************/

void
WickrIOJScriptThread::slotMessageReceived(const QList<QByteArray>& messages)
{
//    qDebug() << "entered slotMessageReceived!";
    for (QByteArray mesg : messages) {
        QString response = processRequest(mesg);

        QList<QByteArray> reply;
        reply += response.toLocal8Bit();
        if (response.length() > 0)
            qDebug() << "Replier::sendReply> " << response;
        m_zsocket->sendMessage(reply);
    }
}

QString
WickrIOJScriptThread::processRequest(const QByteArray& request)
{
    QJsonParseError jsonError;
    QJsonDocument jsonRequest = QJsonDocument().fromJson(request, &jsonError);
    if (jsonError.error != jsonError.NoError) {
        return "Parsing error!";
    }

    QJsonObject object = jsonRequest.object();

    if (!object.contains("action")) {
        return "Malformed request: no action!";
    }

    QJsonValue value;
    QString action;

    value = object["action"];
    action = value.toString().toLower();

    OperationData* operation = WickrIOClientRuntime::operationData();
    if (!operation) {
        return "Internal failure!";
    }
    WickrIOAPIInterface apiInterface(operation);

    QString responseString;

    // MESSAGE ACTIONS
    if (action == "send_message") {
        if (apiInterface.sendMessage(request, responseString)) {
            responseString = "Sending message";
        }
    }
    else if (action == "send_message_uname") {
        if (apiInterface.sendMessage(request, responseString)) {
            responseString = "Sending message";
        }
    }
    else if (action == "get_received_messages") {
        apiInterface.getReceivedMessages(responseString);
    }
    // ROOM ACTIONS
    else if (action == "add_room") {
        apiInterface.addRoom(request, responseString);
    }
    else if (action == "modify_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.updateRoom(vGroupID, request, responseString);
    }
    else if (action == "delete_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.deleteRoom(vGroupID, responseString);
    }
    else if (action == "leave_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.leaveRoom(vGroupID, responseString);
    }
    else if (action == "get_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.getRoom(vGroupID, responseString);
    }
    else if (action == "get_rooms") {
        apiInterface.getRooms(responseString);
    }
    // GROUP CONVERSATION ACTIONS
    else if (action == "add_groupconvo") {
        apiInterface.addGroupConvo(request, responseString);
    }
    else if (action == "delete_groupconvo") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.deleteGroupConvo(vGroupID, responseString);
    }
    else if (action == "get_groupconvo") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.getGroupConvo(vGroupID, responseString);
    }
    else if (action == "get_groupconvos") {
        apiInterface.getGroupConvos(responseString);
    }
    // STATISTICS ACTIONS
    else if (action == "get_statistics") {
        apiInterface.getStatistics("", responseString);
    }
    else if (action == "clear_statistics") {
        apiInterface.clearStatistics("", responseString);
    }
    // Asynchronous message and event handling
    else if (action == "start_async_messages") {
        if (apiInterface.startAsyncMessages(responseString)) {
            WickrIOAddonAsyncService *asyncSvc = (WickrIOAddonAsyncService*)WickrIOClientRuntime::findService(WickrIOAddonAsyncService::asyncServiceBaseName);
            if (asyncSvc) {
                asyncSvc->setAsyncMessagesState(true);
            }
        }
    } else if (action == "stop_async_messages") {
        if (apiInterface.stopAsyncMessages(responseString)) {
            WickrIOAddonAsyncService *asyncSvc = (WickrIOAddonAsyncService*)WickrIOClientRuntime::findService(WickrIOAddonAsyncService::asyncServiceBaseName);
            if (asyncSvc) {
                asyncSvc->setAsyncMessagesState(false);
            }
        }
    } else if (action == "start_async_events") {
        if (apiInterface.startAsyncEvents(responseString)) {
            WickrIOAddonAsyncService *asyncSvc = (WickrIOAddonAsyncService*)WickrIOClientRuntime::findService(WickrIOAddonAsyncService::asyncServiceBaseName);
            if (asyncSvc) {
                asyncSvc->setAsyncEventsState(true);
            }
        }
    } else if (action == "stop_async_events") {
        if (apiInterface.stopAsyncEvents(responseString)) {
            WickrIOAddonAsyncService *asyncSvc = (WickrIOAddonAsyncService*)WickrIOClientRuntime::findService(WickrIOAddonAsyncService::asyncServiceBaseName);
            if (asyncSvc) {
                asyncSvc->setAsyncEventsState(false);
            }
        }
    } else if (action == "encrypt_string") {
        if (!object.contains("string")) {
            return "Malformed request: no string!";
        }
        value = object["string"];
        QString string2encrypt = value.toString();

        WickrStatus status(0);
        QByteArray encryptedBytes = encryptUserDataString(string2encrypt, status);
        if (status.isError()) {
            return "Error encrypting string!";
        }
        QString encryptedString = encryptedBytes.toHex();
        return encryptedString;
    } else if (action == "decrypt_string") {
        if (!object.contains("string")) {
            return "Malformed request: no string!";
        }
        value = object["string"];
        QString string2encrypt = value.toString();
        QByteArray hexBytes = QByteArray::fromHex(string2encrypt.toLatin1());
        WickrStatus status(0);
        QString decryptedString = decryptUserDataString(hexBytes, status);
        if (status.isError()) {
            return "Error decrypting string!";
        }
        return decryptedString;
    }
    else {
        responseString = QString("action '%1' unknown!").arg(action);
    }

    return responseString;
}

