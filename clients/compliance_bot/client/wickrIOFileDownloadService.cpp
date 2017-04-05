#include <QJsonDocument>

#include "wickrIOFileDownloadService.h"
#include "wickrIOClientRuntime.h"

#include "common/wickrRuntime.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
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

    WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
    if (cloudMgr) {
        connect(cloudMgr, &WickrCore::WickrCloudTransferMgr::statusChanged,
                m_fdThread, &WickrIOFileDownloadThread::slotSendFileStatusChange, Qt::QueuedConnection);
    }

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

    // Perform shutdown here, wait for resources to quit, and cleanup
    WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
    if (cloudMgr) {
        disconnect(cloudMgr, &WickrCore::WickrCloudTransferMgr::statusChanged,
                m_fdThread, &WickrIOFileDownloadThread::slotSendFileStatusChange);
    }

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

    m_activeDownloads.insert(file_guid, dload);

    WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
    if (cloudMgr) {
        if (! dload->m_downloaded) {
            if (dload->m_downloading) {
                // Check if the download has completed
                QFileInfo f(dload->m_attachmentFileName);
                if (f.exists()) {
                    dload->m_downloaded = true;
                    dload->m_downloading = false;
                }
            } else {
                dload->m_downloading = true;
                cloudMgr->downloadFile(nullptr, dload->m_attachmentFileName, dload->m_fileInfo);
            }
        }
    } else {
        // Failed to get cloud manager so lets trash what we have
        m_activeDownloads.remove(file_guid);
    }

#if 0
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_state = CBThreadState::CB_STARTED;
    } else {
        WickrIOAppSettings appSetting;

        if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
            QString url = appSetting.value;
            if (! url.isEmpty()) {
                startUrlCallback(url);
            } else {
                m_state = CBThreadState::CB_STARTED;
            }
        } else if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
            WickrIOEmailSettings *email = new WickrIOEmailSettings();
            if (appSetting.getEmail(email)) {
                startEmailCallback(email);
            }
            delete email;
            m_state = CBThreadState::CB_STARTED;
        } else {
            m_state = CBThreadState::CB_STARTED;
        }
    }
#endif
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

            int msgID = db->insertMessage(dload->m_msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(), (int)dload->m_msg->getMsgClass(), 1);
            db->insertAttachment(msgID, dload->m_attachmentFileName, dload->m_realFileName);
            m_activeDownloads.remove(uuid);

            dload->m_msg->doDelete();
            dload->m_msg->release();

            delete dload;
        }
    }
}

