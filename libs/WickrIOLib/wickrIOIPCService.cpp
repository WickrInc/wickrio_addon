#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "wickrIOIPCService.h"
#include "wickrbotprocessstate.h"
#include "wickrIOCommon.h"

WickrIOIPCService::WickrIOIPCService() :
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_ipcThread(nullptr)
{
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
    m_ipcThread = new WickrIOIPCThread(&m_thread, this);

    // connect the thread signals to the service signals
    connect(m_ipcThread, &WickrIOIPCThread::signalGotPauseRequest, this, &WickrIOIPCService::signalGotPauseRequest);
    connect(m_ipcThread, &WickrIOIPCThread::signalGotStopRequest, this, &WickrIOIPCService::signalGotStopRequest);
    connect(m_ipcThread, &WickrIOIPCThread::signalReceivedMessage, this, &WickrIOIPCService::signalReceivedMessage);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

bool WickrIOIPCService::isRunning()
{
    if (m_ipcThread != nullptr) {
        return m_ipcThread->isRunning();
    }
    return false;
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
    connect(m_ipcThread, &WickrIOIPCThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdown();

    qDebug() << "WickrIOIPC Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "WickrIOIPC Service shutdown: finished waiting";
}

bool
WickrIOIPCService::check()
{
    QEventLoop wait_loop;
    connect(m_ipcThread, &WickrIOIPCThread::signalIPCCheckDone, &wait_loop, &QEventLoop::quit);
    emit signalIPCCheck();

    // Wait for the thread to signal it is done
    wait_loop.exec();
    return m_ipcThread->ipcCheck();
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOIPCThread::WickrIOIPCThread(QThread *thread, WickrIOIPCService *ipcSvc) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(ipcSvc),
    m_operation(nullptr),
    m_ipcCheck(false)
{
    thread->setObjectName("WickrIOIPCThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Catch the shutdown signal
    connect(ipcSvc, &WickrIOIPCService::signalShutdown, this, &WickrIOIPCThread::slotShutdown);
    connect(ipcSvc, &WickrIOIPCService::signalStartIPC, this, &WickrIOIPCThread::slotStartIPC);
    connect(ipcSvc, &WickrIOIPCService::signalIPCCheck, this, &WickrIOIPCThread::slotIPCCheck);
}

/**
 * @brief WickrIOIPCThread::~WickrIOIPCThread
 * Destructor
 */
WickrIOIPCThread::~WickrIOIPCThread() {
    qDebug() << "WickrIOIPC THREAD: Worker Destroyed.";
}

void
WickrIOIPCThread::slotShutdown()
{
    emit signalNotRunning();
}

void
WickrIOIPCThread::slotIPCCheck()
{
    if (m_ipc != NULL) {
        m_ipcCheck = m_ipc->check();
    } else {
        m_ipcCheck = true;
    }

    emit signalIPCCheckDone();
}

void
WickrIOIPCThread::slotStartIPC(OperationData *operation)
{
    m_operation = operation;
    m_operation->log("Started WickrIOIPCThread");

    m_ipc = new WickrBotIPC();
    m_ipc->startServer();

    m_operation->log(QString("server port=%1").arg(m_ipc->getServerPort()));

    if (m_operation->m_botDB != NULL && m_operation->m_botDB->isOpen()) {
        m_operation->m_botDB->setProcessIPC(m_operation->processName, m_ipc->getServerPort());
    } else {
        m_operation->log("WickrIOIPCThread: database is not open yet!");
    }

    m_operation->log("WickrIOIPCThread: processStartedA");
    QObject::connect(m_ipc, &WickrBotIPC::signalGotMessage, [=](const QString &message, int peerPort) {
        Q_UNUSED(peerPort);
        if (message == WBIO_IPCCMDS_STOP) {
            m_operation->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_STOP));
            m_operation->log("WickrIOIPCThread::processStarted: QUITTING");
            emit signalGotStopRequest();
        } else if (message == WBIO_IPCCMDS_PAUSE) {
            m_operation->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_PAUSE));
            m_operation->log("WickrIOIPCThread::processStarted: PAUSING");
            emit signalGotPauseRequest();
        } else {
            QStringList pieces = message.split("=");
            if (pieces.size() == 2) {
                QString type = pieces.at(0);
                QString value = pieces.at(1);
                m_operation->log(QString("GOT MESSAGE: %1").arg(type));
                emit signalReceivedMessage(type, value);
            } else {
                m_operation->log("GOT MESSAGE: invalid message:" + message);
            }
        }
    });
    m_operation->log("WickrIOIPCThread: processStartedB");
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


