#include <QJsonDocument>

#include "wickrIOFileDownloadService.h"
#include "wickrIOClientRuntime.h"

#include "common/wickrRuntime.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "wickriodatabase.h"


WickrIOFileDownloadService::WickrIOFileDownloadService() :
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_fdThread(nullptr)
{
    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads();

    setObjectName("WickrIOFileDownloadThread");
    qDebug() << "WICKRIOFILEDOWNLOAD SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIOFileDownloadService::~WickrIOFileDownloadService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOFILEDOWNLOAD SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOFileDownloadService::startThreads
 * Starts all threads on message service.
 */
void WickrIOFileDownloadService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_fdThread = new WickrIOFileDownloadThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOFileDownloadService::signalDownloadFile,
            m_fdThread, &WickrIOFileDownloadThread::slotDownloadFile, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOFileDownloadService::stopThreads
 * Stops all threads on switchboard service.
 */
void WickrIOFileDownloadService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOFILEDOWNLOAD THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOFileDownloadService::downloadFile(WickrIORxDownloadFile *dload)
{
    emit signalDownloadFile(dload);
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


WickrIOFileDownloadThread::WickrIOFileDownloadThread(QThread *thread, WickrIOFileDownloadService *dfSvc) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(dfSvc),
    m_running(false),
    m_operation(nullptr)
{
    thread->setObjectName("WickrIOFileDownloadThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    WickrFileTransferService *ftsvc = WickrCore::WickrRuntime::ftSvc();
    if (ftsvc) {
        connect(ftsvc, &WickrFileTransferService::statusChanged,
                this,  &WickrIOFileDownloadThread::slotSendFileStatusChange,
                Qt::QueuedConnection);
    } else {
        qDebug() << "Could not access the File Transfer service!";
    }

    m_running = true;
}

/**
 * @brief WickrIOFileDownloadThread::~WickrIOFileDownloadThread
 * Destructor
 */
WickrIOFileDownloadThread::~WickrIOFileDownloadThread() {
    qDebug() << "WICKRIOFILEDOWNLOAD THREAD: Worker Destroyed.";
}

void
WickrIOFileDownloadThread::slotDownloadFile(WickrIORxDownloadFile *dload)
{
    QWriteLocker lockGuard(&m_lock);

    if (m_operation == nullptr) {
        m_operation = WickrIOClientRuntime::operationData();
    }

    QString file_guid = dload->m_fileInfo.metaData().fetchInfo().at(0).guid;
    QByteArray file_key = dload->m_fileInfo.metaData().fetchInfo().at(0).key;

    if (!WickrCore::WickrRuntime::ftScheduleDownload(file_guid, file_key, dload->m_attachmentFileName, true, true)) {
        qDebug() << "Could not schedule download for" << dload->m_attachmentFileName;
        // TODO: Need to reschedule this, for now release the message
        // Release the message
        dload->m_msg->dodelete(traceInfo());
        dload->m_msg->release();
        delete dload;
    } else {
        m_activeDownloads.insert(file_guid, dload);
    }
}

void
WickrIOFileDownloadThread::slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName)
{
    QWriteLocker lockGuard(&m_lock);

    if (status == "complete") {
        WickrIORxDownloadFile *dload = m_activeDownloads.value(uuid);

        // If done downloading and decrypting then pass off
        if (dload != NULL) {
            WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);

            QJsonDocument saveDoc(dload->m_json);

            int msgID = db->insertMessage(dload->m_msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(QJsonDocument::Compact), (int)dload->m_msg->getMsgClass(), 1);
            db->insertMsgAttachment(msgID, dload->m_attachmentFileName, dload->m_realFileName);
            m_activeDownloads.remove(uuid);

            dload->m_msg->dodelete(traceInfo());
            dload->m_msg->release();

            delete dload;
        }
    }
}

