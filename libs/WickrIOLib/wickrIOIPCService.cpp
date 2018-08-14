#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "nzmqt/nzmqt.hpp"

#include "wickrIOIPCService.h"
#include "wickrbotprocessstate.h"
#include "wickrIOCommon.h"

WickrIOIPCService::WickrIOIPCService(const QString& name, bool isClient) :
    m_lock(QReadWriteLock::Recursive),
    m_name(name),
    m_isClient(isClient),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED)
{
    m_wickrID = m_name;
    m_name.replace("@", "_");

    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads();

    setObjectName("WickrIOIPCService");
    qDebug() << "WickrIOIPC SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIOIPCService::~WickrIOIPCService() {
    // Stop threads
    stopThreads();

    qDebug() << "WickrIOIPC SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOIPCService::startThreads
 * Starts all threads on message service.
 */
void WickrIOIPCService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_ipcRxThread = new WickrIOIPCRecvThread(&m_thread, this);
    m_ipcTxThread = new WickrIOIPCSendThread(&m_thread, this);

    // connect the thread signals to the service signals
    connect(m_ipcRxThread, &WickrIOIPCRecvThread::signalGotPauseRequest, this, &WickrIOIPCService::signalGotPauseRequest);
    connect(m_ipcRxThread, &WickrIOIPCRecvThread::signalGotStopRequest,  this, &WickrIOIPCService::signalGotStopRequest);
    connect(m_ipcRxThread, &WickrIOIPCRecvThread::signalReceivedMessage, this, &WickrIOIPCService::signalReceivedMessage);

    connect(m_ipcTxThread, &WickrIOIPCSendThread::signalRequestSent,     this, &WickrIOIPCService::signalMessageSent);
    connect(m_ipcTxThread, &WickrIOIPCSendThread::signalRequestFailed,   this, &WickrIOIPCService::signalMessageSendFailure);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

bool WickrIOIPCService::isRunning()
{
    if (m_ipcRxThread == nullptr  || m_ipcTxThread == nullptr ||
        !m_ipcRxThread->isRunning() || !m_ipcTxThread->isRunning()) {
        return false;
    }
    return true;
}

void WickrIOIPCService::startIPC(OperationData *operation)
{
    emit signalStartIPC(operation);
}


/**
 * @brief WickrIOIPCService::stopThreads
 * Stops all threads on switchboard service.
 */
void WickrIOIPCService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WickrIOIPC THREAD: Shutdown Thread (%p)", &m_thread);
}

/**
 * @brief WickrIOIPCService::shutdown
 * Called to shutdown the watchdog service.
 */
void WickrIOIPCService::shutdown()
{
    QEventLoop wait_loop;
    connect(m_ipcRxThread, &WickrIOIPCRecvThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdownRecv();

    qDebug() << "WickrIORecvIPC Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "WickrIORecvIPC Service shutdown: finished waiting";

    QEventLoop wait_loop_tx;
    connect(m_ipcTxThread, &WickrIOIPCSendThread::signalNotRunning, &wait_loop_tx, &QEventLoop::quit);
    emit signalShutdownSend();

    qDebug() << "WickrIOSendIPC Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop_tx.exec();
    qDebug() << "WickrIOSendIPC Service shutdown: finished waiting";
}

bool WickrIOIPCService::sendMessage(const QString& dest, bool isClient, const QString& message)
{
    emit signalSendMessage(dest, isClient, message);
    return true;
}

void WickrIOIPCService::closeClientConnection(const QString& dest)
{
    emit signalCloseConnection(dest);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOIPCRecvThread::WickrIOIPCRecvThread(QThread *thread, WickrIOIPCService *ipcSvc) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(ipcSvc)
{
    thread->setObjectName("WickrIOIPCRecvThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Catch the shutdown signal
    connect(ipcSvc, &WickrIOIPCService::signalShutdownRecv, this, &WickrIOIPCRecvThread::slotShutdown);
    connect(ipcSvc, &WickrIOIPCService::signalStartIPC, this, &WickrIOIPCRecvThread::slotStartIPC);
}

/**
 * @brief WickrIOIPCRecvThread::~WickrIOIPCRecvThread
 * Destructor
 */
WickrIOIPCRecvThread::~WickrIOIPCRecvThread() {
    qDebug() << "WickrIOIPC THREAD: Worker Destroyed.";
}

void
WickrIOIPCRecvThread::slotShutdown()
{
    if (m_zsocket != nullptr) {
        m_zsocket->close();
        m_zsocket = nullptr;
    }
    if (m_zctx != nullptr && !m_zctx->isStopped()) {
        m_zctx->stop();
        m_zctx->deleteLater();
    }
    m_zctx = nullptr;

    emit signalNotRunning();
}

void
WickrIOIPCRecvThread::slotStartIPC(OperationData *operation)
{
    m_operation = operation;
    if (m_operation != nullptr)
        m_operation->log_handler->log("Started WickrIOIPCRecvThread");

    m_zctx = nzmqt::createDefaultContext();

    m_zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REP, this);
    m_zsocket->setObjectName(QString("%1.Socket.socket(REP)").arg(m_parent->m_name));
    m_zsocket->setOption(nzmqt::ZMQSocket::OPT_LINGER, 100);
    connect(m_zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &WickrIOIPCRecvThread::slotMessageReceived, Qt::QueuedConnection);

    m_zctx->start();

    QString queueDirName;
    QString queueName;
    QString queueFileName;

    if (m_parent->m_isClient) {
        queueDirName = QString(WBIO_IPCCLIENT_SOCKETDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    } else {
        queueDirName = QString(WBIO_IPCSERVER_SOCKETDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    }
    QDir queueDir(queueDirName);
    if (!queueDir.exists()) {
        if (!queueDir.mkpath(queueDirName)) {
            qDebug() << "Cannot create message queue directory!";
        }
    }
    if (m_parent->m_isClient) {
        queueName = QString(WBIO_IPCCLIENT_RXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    } else {
        queueName = QString(WBIO_IPCSERVER_RXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    }
    m_zsocket->bindTo(queueName);

    // Set the permission of the queue file so that normal user programs can access
    if (m_parent->m_isClient) {
        queueFileName = QString(WBIO_IPCCLIENT_SOCKETFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    } else {
        queueFileName = QString(WBIO_IPCSERVER_SOCKETFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_parent->m_name);
    }
    QFile zmqFile(queueFileName);
    if(!zmqFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                               QFile::ReadUser | QFile::WriteUser | QFile::ExeUser  |
                               QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                               QFile::ReadOther | QFile::WriteOther | QFile::ExeOther)) {
        qDebug("Something wrong setting permissions of the queue file!");
    }

}

void
WickrIOIPCRecvThread::slotMessageReceived(const QList<QByteArray>& messages)
{
    qDebug() << "entered slotMessageReceived!";
    for (QByteArray message : messages) {
        // Send reply before processing the received message
        QList<QByteArray> reply;
        reply += "Message received";
        m_zsocket->sendMessage(reply);

        if (message == WBIO_IPCCMDS_STOP) {
            if (m_operation != nullptr) {
                m_operation->log_handler->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_STOP));
                m_operation->log_handler->log("WickrIOIPCRecvThread::processStarted: QUITTING");
            }
            emit signalGotStopRequest();
        } else if (message == WBIO_IPCCMDS_PAUSE) {
            if (m_operation != nullptr) {
                m_operation->log_handler->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_PAUSE));
                m_operation->log_handler->log("WickrIOIPCRecvThread::processStarted: PAUSING");
            }
            emit signalGotPauseRequest();
        } else {
            QStringList pieces = QString(message).split('=');
            if (pieces.size() == 2) {
                QString type = pieces.at(0);
                QString value = pieces.at(1);
                if (m_operation != nullptr) {
                    m_operation->log_handler->log(QString("GOT MESSAGE: %1").arg(type));
                }
                emit signalReceivedMessage(type, value);
            } else {
                if (m_operation != nullptr) {
                    m_operation->log_handler->log("GOT MESSAGE: invalid message:" + message);
                }
            }
        }

    }
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOIPCSendThread::WickrIOIPCSendThread(QThread *thread, WickrIOIPCService *ipcSvc) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(ipcSvc)
{
    thread->setObjectName("WickrIOIPCSendThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Catch the shutdown signal
    connect(ipcSvc, &WickrIOIPCService::signalShutdownSend, this, &WickrIOIPCSendThread::slotShutdown);
    connect(ipcSvc, &WickrIOIPCService::signalStartIPC, this, &WickrIOIPCSendThread::slotStartIPC);
    connect(ipcSvc, &WickrIOIPCService::signalSendMessage, this, &WickrIOIPCSendThread::slotSendMessage);
    connect(ipcSvc, &WickrIOIPCService::signalCloseConnection, this, &WickrIOIPCSendThread::slotCloseConnection);
}

/**
 * @brief WickrIOIPCSendThread::~WickrIOIPCSendThread
 * Destructor
 */
WickrIOIPCSendThread::~WickrIOIPCSendThread() {
    qDebug() << "WickrIOIPC THREAD: Worker Destroyed.";
}

void
WickrIOIPCSendThread::slotShutdown()
{
    if (m_zctx != nullptr && !m_zctx->isStopped()) {
        m_zctx->stop();
        m_zctx->deleteLater();
    }
    m_zctx = nullptr;    emit signalNotRunning();
}

void
WickrIOIPCSendThread::slotStartIPC(OperationData *operation)
{
    m_operation = operation;
    if (m_operation != nullptr) {
        m_operation->log_handler->log("Started WickrIOIPCSendThread");
    }

    m_zctx = nzmqt::createDefaultContext();
    m_zctx->start();
}

nzmqt::ZMQSocket *
WickrIOIPCSendThread::createSendSocket(const QString& dest, bool isClient)
{
    nzmqt::ZMQSocket *zsocket;

    zsocket = m_txMap.value(dest);

    // iff the socket exists return it
    if (zsocket != nullptr) {
        return zsocket;
    }

    zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REQ, this);
    zsocket->setObjectName(QString("%1.Socket.socket(REQ)").arg(dest));
    zsocket->setOption(nzmqt::ZMQSocket::OPT_LINGER, 100);
    connect(zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &WickrIOIPCSendThread::slotMessageReceived, Qt::QueuedConnection);

    QString queueName;
    if (isClient) {
        queueName = QString(WBIO_IPCCLIENT_RXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(dest);
    } else {
        queueName = QString(WBIO_IPCSERVER_RXSOCKET_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(dest);
    }

    zsocket->connectTo(queueName);

    m_txMap.insert(dest, zsocket);
    return zsocket;
}

void
WickrIOIPCSendThread::slotSendMessage(const QString& dest, bool isClient, const QString& message)
{
    nzmqt::ZMQSocket *zsocket = createSendSocket(dest, isClient);
    if (zsocket != nullptr) {
        if (! zsocket->isConnected()) {
            qDebug() << "Socket is NOT connected!";
            m_txMap.remove(dest);
            zsocket = createSendSocket(dest, isClient);
            if (zsocket == nullptr) {
                qDebug().nospace().noquote() << "Cannot create Socket for " << dest;
                return;
            }
        }
        QByteArray request = message.toLocal8Bit();
        zsocket->sendMessage(request);
        qDebug().nospace().noquote() << "WickrIOIPCSendThread::slotSendMessage: Sent msg to " << dest;

        emit signalRequestSent();
    } else {
        emit signalRequestFailed();
    }
}


void
WickrIOIPCSendThread::slotMessageReceived(const QList<QByteArray>& messages)
{
    for (QByteArray message : messages) {
        qDebug() << "WickrIOIPCSendThread::slotMessageReceived: received response:" << message;
    }
}

void
WickrIOIPCSendThread::slotCloseConnection(const QString& dest)
{
    if (m_txMap.count() > 0 && m_txMap.contains(dest)) {
        nzmqt::ZMQSocket *zsocket = m_txMap.value(dest);
        if (zsocket != nullptr) {
            zsocket->close();
        }

        m_txMap.remove(dest);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

QString
WickrIOIPCCommands::getBotInfoString(const QString& sendingProc, const QString& name, const QString& procName, const QString& password)
{
    QString string = QString("%1={\"%2\":\"%3\", \"%4\":\"%5\", \"%6\":\"%7\", \"%8\":\"%9\"}")
            .arg(WBIO_IPCMSGS_BOTINFO)
            .arg(WBIO_IPCHDR_PROCESSNAME).arg(sendingProc)
            .arg(WBIO_BOTINFO_CLIENT).arg(name)
            .arg(WBIO_BOTINFO_PROCESSNAME).arg(procName)
            .arg(WBIO_BOTINFO_PASSWORD).arg(password);
    return string;
}

QMap<QString,QString>
WickrIOIPCCommands::parseBotInfoValue(const QString& value)
{
    QMap<QString,QString> retMap;

    QJsonParseError jsonError;
    QByteArray valueByteArray = value.toUtf8();
    QJsonDocument jsonResponse = QJsonDocument().fromJson(valueByteArray, &jsonError);

    // Check if there is a JSON parsing error
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "parseBotInfoValue:" << jsonError.errorString();
        return retMap;
    }

    QJsonObject jsonObject = jsonResponse.object();

    if (! jsonObject.contains(WBIO_IPCHDR_PROCESSNAME) ||
        ! jsonObject.contains(WBIO_BOTINFO_CLIENT) ||
        ! jsonObject.contains(WBIO_BOTINFO_PROCESSNAME) ||
        ! jsonObject.contains(WBIO_BOTINFO_PASSWORD)) {
        qDebug() << "parseBotInfoValue: missing a needed value!";
        return retMap;
    }

    QJsonValue jsonValue;

    // get the sending process name value
    jsonValue = jsonObject[WBIO_IPCHDR_PROCESSNAME];
    retMap.insert(WBIO_IPCHDR_PROCESSNAME, jsonValue.toString(""));

    // get the client value
    jsonValue = jsonObject[WBIO_BOTINFO_CLIENT];
    retMap.insert(WBIO_BOTINFO_CLIENT, jsonValue.toString(""));

    // get the process name value
    jsonValue = jsonObject[WBIO_BOTINFO_PROCESSNAME];
    retMap.insert(WBIO_BOTINFO_PROCESSNAME, jsonValue.toString(""));

    // get the client value
    jsonValue = jsonObject[WBIO_BOTINFO_PASSWORD];
    retMap.insert(WBIO_BOTINFO_PASSWORD, jsonValue.toString(""));

    return retMap;
}

QString
WickrIOIPCCommands::getPasswordString(const QString& sendingProc, const QString& password)
{
    QString string = QString("%1={\"%2\":\"%3\", \"%4\":\"%5\"}")
            .arg(WBIO_IPCMSGS_PASSWORD)
            .arg(WBIO_IPCHDR_PROCESSNAME).arg(sendingProc)
            .arg(WBIO_BOTINFO_PASSWORD).arg(password);
    return string;
}

QMap<QString,QString>
WickrIOIPCCommands::parsePasswordValue(const QString& value)
{
    QMap<QString,QString> retMap;

    QJsonParseError jsonError;
    QByteArray valueByteArray = value.toUtf8();
    QJsonDocument jsonResponse = QJsonDocument().fromJson(valueByteArray, &jsonError);

    // Check if there is a JSON parsing error
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "parsePasswordValue:" << jsonError.errorString();
        return retMap;
    }

    QJsonObject jsonObject = jsonResponse.object();

    if (! jsonObject.contains(WBIO_IPCHDR_PROCESSNAME) ||
        ! jsonObject.contains(WBIO_BOTINFO_PASSWORD)) {
        qDebug() << "parsePasswordValue: missing a needed value!";
        return retMap;
    }

    QJsonValue jsonValue;

    // get the sending process name value
    jsonValue = jsonObject[WBIO_IPCHDR_PROCESSNAME];
    retMap.insert(WBIO_IPCHDR_PROCESSNAME, jsonValue.toString(""));

    // get the client value
    jsonValue = jsonObject[WBIO_BOTINFO_PASSWORD];
    retMap.insert(WBIO_BOTINFO_PASSWORD, jsonValue.toString(""));

    return retMap;
}


