#include "coreIpcService.h"

QString CoreIpcService::jsServiceBaseName = "WickrIOJScriptThread";

CoreIpcService::CoreIpcService(QObject *parent) : QObject(parent),
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED)
{
    qRegisterMetaType<WickrServiceState>("CoreIpcService");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads();

    setObjectName("CoreIpcThread");
    qDebug() << "COREIPC SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

CoreIpcService::~CoreIpcService() {
    // Stop threads
    stopThreads();

    qDebug() << "COREIPC SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

void CoreIpcService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new CoreIpcThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &CoreIpcService::signalStartIpcListening,
            m_cbThread, &CoreIpcThread::slotStartIpcListening, Qt::QueuedConnection);

    connect(m_cbThread, &CoreIpcThread::signalStateChange, this, &CoreIpcService::signalStateChange);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

void CoreIpcService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("COREIPC SERVICE: Shutdown Thread (%p)", &m_thread);
}

void CoreIpcService::startListening()
{
    emit signalStartIpcListening();
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

CoreIpcThread::CoreIpcThread(QThread *thread, CoreIpcService *ipcSvc, QObject *parent)
    : QObject(parent),
      m_parent(ipcSvc)
    , m_state(JSThreadState::JS_UNINITIALIZED)
{
    thread->setObjectName("CoreIpcThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    m_state = JSThreadState::JS_STARTED;
}

CoreIpcThread::~CoreIpcThread() {
    qDebug() << "COREIPC THREAD: Worker Destroyed.";
}

void
CoreIpcThread::slotStartIpcListening()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;
    m_state = JSThreadState::JS_PROCESSING;

    m_zctx = nzmqt::createDefaultContext();

    m_zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REP, this);
    m_zsocket->setObjectName("Replier.Socket.socket(REP)");
    connect(m_zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &CoreIpcThread::slotMessageReceived, Qt::QueuedConnection);

    m_zctx->start();

    QString queueDirName = QString("%1/tmp").arg(QDir::currentPath());
    QDir    queueDir(queueDirName);
    if (!queueDir.exists()) {
        if (!queueDir.mkpath(queueDirName)) {
            qDebug() << "Cannot create message queue directory!";
        }
    }

    // Create the socket file for the addon receive queue
    {
        QString queueName = QString("ipc://%1/tmp/0").arg(QDir::currentPath());
        m_zsocket->bindTo(queueName);

        // Set the permission of the queue file so that normal user programs can access
        QString queueFileName = QString("%1/tmp/0").arg(QDir::currentPath());
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


void
CoreIpcThread::slotMessageReceived(const QList<QByteArray>& messages)
{
    for (QByteArray mesg : messages) {
        QString response = "success";
        QList<QByteArray> reply;
        reply += response.toLocal8Bit();
        if (response.length() > 0)
            qDebug() << "Replier::sendReply> " << response;
        m_zsocket->sendMessage(reply);

        // Process the inbound message
        QString message(mesg);
        if (message.toLower() == "shutdown") {
            emit signalStateChange(true);
        }
    }
}

