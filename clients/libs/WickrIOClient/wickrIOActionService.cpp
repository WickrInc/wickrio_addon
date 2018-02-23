#include <QUuid>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include "wickrIOActionService.h"
#include "Wickr/aes/AESHelper.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "requests/wickrUserRequests.h"
#include "wickrbotactiondatabase.h"

WickrIOActionService::WickrIOActionService(OperationData *operation) : WickrIOServiceBase("WickrIOActionThread"),
    m_lock(QReadWriteLock::Recursive),
    m_ahThread(nullptr)
{
    m_state = WickrServiceState::SERVICE_UNINITIALIZED;

    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads(operation);

    setObjectName("WickrIOActionThread");
    qDebug() << "WICKRIOACTION SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIOActionService::~WickrIOActionService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOACTION SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WIckrIOActionService::startThreads
 * Starts all threads on message service.
 */
void WickrIOActionService::startThreads(OperationData *operation)
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_ahThread = new WickrIOActionThread(&m_thread, this, operation);

    // connect to signals
    connect(m_ahThread, &WickrIOActionThread::signalProcessAction, this, &WickrIOActionService::slotProcessAction);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WIckrIOActionService::stopThreads
 * Stops all threads on action handler service.
 */
void WickrIOActionService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOACTION THREAD: Shutdown Thread (%p)", &m_thread);
}

/**
 * @brief WIckrIOActionService::shutdown
 * Called to shutdown the action handler service.
 */
void WickrIOActionService::shutdown()
{
    QEventLoop wait_loop;
    connect(m_ahThread, &WickrIOActionThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdown();

    qDebug() << "Action Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "Action Service shutdown: finished waiting";
}

/**
 * @brief WickrIOActionService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Action Handler this is typically related to a stuck action.
 * @return
 */
bool WickrIOActionService::isHealthy()
{
    if (m_processActionStarted != 0) {
        qint64 curTime = QDateTime::currentSecsSinceEpoch();
        // If a process action is taking more than 20 seconds then fail
        if (curTime - m_processActionStarted > 20)
            return false;
    }
    return true;
}

void WickrIOActionService::slotProcessAction(bool state)
{
    if (state) {
        m_processActionStarted = QDateTime::currentSecsSinceEpoch();
    } else {
        m_processActionStarted = 0;
    }
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


WickrIOActionThread::WickrIOActionThread(QThread *thread, WickrIOActionService *ahSvc, OperationData *operation) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(ahSvc),
    m_running(false),
    m_operation(operation)
{
    thread->setObjectName("WickrIOActionThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
    m_timer.start(1000 * WICKRIO_AH_UPDATE_PROCESS_SECS);

    initCounts();
    m_appCounter.clear();

    this->connect(this, &WickrIOActionThread::signalSendMessageDoneGetUsers, this, &WickrIOActionThread::slotSendMessagePostGetUsers);
    this->connect(this, &WickrIOActionThread::signalStartProcessDatabase, this, &WickrIOActionThread::processDatabase);

    WickrFileTransferService *ftSvc = WickrCore::WickrRuntime::ftSvc();
    if (ftSvc) {
        connect(ftSvc, &WickrFileTransferService::statusChanged, this, &WickrIOActionThread::slotSendFileStatusChange);
    } else {
        qDebug() << "WickrIOActionThread: cannot get file transfer service!";
    }

    // Catch the shutdown signal
    connect(ahSvc, &WickrIOActionService::signalShutdown, this, &WickrIOActionThread::slotShutdown);

    // Catch the register and deregister signals

    m_running = true;
}

/**
 * @brief WickrIOActionThread::~WickrIOActionThread
 * Destructor
 */
WickrIOActionThread::~WickrIOActionThread() {
    m_running = false;

    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));

    qDebug() << "WICKRIOACTION THREAD: Worker Destroyed.";
}

void
WickrIOActionThread::slotShutdown()
{
    // If processing an action then cannot fully stop
    if (processActionState()) {
        m_shuttingdown = true;
        m_shutdownCountDown = 5;
    } else {
        if (m_timer.isActive())
            m_timer.stop();
        m_running = false;

        emit signalNotRunning();
    }
}

/**
 * @brief WickrIOActionThread::doTimerWork
 * This method will perform operations that are to be done when our one second timer
 * goes off. The main signal is to initiate a Process Database operation.
 */
void WickrIOActionThread::slotTimerExpire()
{
    // If the process is not running then don't do anything
    if (!m_running) {
        return;
    }

    // If we are shutting down then make sure to wait for any active processes
    if (m_shuttingdown) {
        if (processActionState()) {
            // If we have waited long enough then force a shutdown.
            if (m_shutdownCountDown-- <= 0) {
                setProcessAction(false);
                slotShutdown();
            }
        }
        return;
    }

    // Increment the App Counter, needed to backup the number of Apps sent to
    m_appCounter.incSecond();
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRIO_AH_UPDATE_STATS_SECS) == 0) {
        m_outputStats = true;
    }

    // Only print out if app count is greater than 0 (reduce output)
    int appCount = m_appCounter.getCumulative(60);
    if (appCount) {
        qDebug() << "Current app count for the last minute=" << appCount;
    }

    if (! m_processCleanUp) {
        if ((m_timerStatsTicker % m_operation->cleanUpSecs) == 0 || m_delayedRcvOrClean) {
            // If actions are bein processed then delay the db clean
            if (processActionState()) {
                m_delayedRcvOrClean = true;
            } else {
                m_delayedRcvOrClean = false;
                cleanUpDatabase();
            }
        }
    }

    // If there are still backoff seconds remaining then decrement and leave
    if (m_backoff > 0) {
        m_backoff--;
    }
    // Else if we are not processing an action and not cleaning up then signal to start an action
    else if (! processActionState()) {
        if (! m_processCleanUp)
            emit signalStartProcessDatabase(WICKRIO_AH_INVALID_ID, true);
    }

}

void
WickrIOActionThread::setProcessAction(bool state)
{
    m_processActionState = state;
    emit signalProcessAction(state);
}








bool
WickrIOActionThread::sendMessageTo1To1(WickrCore::WickrConvo *convo)
{
    // Make sure there are actually users in this 1to1
    if (!convo || convo->getAllUsers().length() == 0) {
        qDebug() << "sendMessageTo1To1: no users in 1to1!";
        return false;
    }

    /*
     * Setup and send the message
     */
    // TODO: Handle the attachment
    QList<QString> attachmentFiles = m_jsonHandler->getAttachments();

    long ttl = m_jsonHandler->getTTL();
    if (ttl == 0) {
        ttl = convo->getDestruct();
    } else {
        if (ttl != convo->getDestruct())
            convo->setDestruct(ttl);
    }

    if (m_jsonHandler->hasBOR()) {
    long bor = m_jsonHandler->getBOR();
        convo->setBOR(bor);
    }

    if (attachmentFiles.size() > 0) {
        if (! sendFile(convo, attachmentFiles, m_jsonHandler->getMessage())) {
            return false;
        }
    } else {
#if 0
        // Send message
        WickrSendContext *context = new WickrSendContext(MsgType_Text,
                                                         convo,
                                                         WickrCore::WickrMessage::createTextMsgBody(m_jsonHandler->getMessage(),convo));
#else
        // Get the users
        QList<WickrCore::WickrUser *> userList;
        QList<WickrCore::WickrUser *> convoUsers = convo->getAllUsers();
        WickrCore::WickrUser *selfUser = WickrCore::WickrUser::getSelfUser();
        for (WickrCore::WickrUser *user : convoUsers) {
            if (user != selfUser) {
                userList.append(user);
            }
        }

        // Send message
        WickrSendContext *context = new WickrSendContext(MsgType_Text,
                                                         convo,
                                                         WickrCore::WickrMessage::createTextMsgBody(m_jsonHandler->getMessage(),convo),
                                                         userList);
#endif
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionThread::slotMessageDone, Qt::QueuedConnection);
        if (!WickrCore::WickrRuntime::msgSvcSend(context)) {
            qDebug() << "IN sendMessageTo1To1(): msgSvcSend returned failure!";
            return false;
        }
    }

    // Free the JSON Handler object
    delete m_jsonHandler;
    m_jsonHandler = NULL;
    return true;
}


/**
 * @brief WickrIOActionThread::processActionSendMessage
 * This function will handle the Send Message action. The input jsonHandler argument will
 * contain all of the necessary information to identify who to send the message to and
 * the actual message.
 * @param jsonHandler Details of the message to be sent
 * @param actionID The ID of the action from the database. Need to remove after success.
 */
bool WickrIOActionThread::processActionSendMessage(WickrBotJson *jsonHandler, int actionID)
{
//    QList<WickrCore::WickrUser *> wickrUsers;
    m_wickrUsers.clear();
    m_jsonHandler = jsonHandler;

    // Save the action ID. It is IMPORTANT that sends are not sent at the same time.
    m_curActionID = actionID;

    if (jsonHandler->getUserIDs().size() == 0 && jsonHandler->getUserNames().size() == 0) {
        QString vGroupID = jsonHandler->getVGroupID();
        if (vGroupID.isEmpty()) {
            m_operation->log_handler->error("Send message has no users or vgroupid");
            return false;
        }

        WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
        if ( convo == 0 ) {
            m_operation->log_handler->error("cannot convo with vgroupid = " + vGroupID);
            m_messagesFailed++;
            return false;
        }
        if (convo->getConvoType() == CONVO_ONE_TO_ONE) {
            return sendMessageTo1To1(convo);
        } else {
            return sendMessageToConvo(convo);
        }
    }

    for (QString userID : jsonHandler->getUserIDs()) {
        WickrStatus status(0);

        QString userIDHashSecure = WickrCore::WickrUser::activeSessionGetSecureAlias(userID, status);
        if (status.isError()) {
            m_operation->log_handler->error("Error hashing userID!");
            m_messagesFailed++;
            return false;
        }

        QByteArray userIDSecure = encryptUserDataString(userID, status);
        if (status.isError()) {
            m_operation->log_handler->error("Error encrypting userID!");
            m_messagesFailed++;
            return false;
        }


        WickrCore::WickrUser *user;
        user = WickrCore::WickrUser::getUserWithID(userIDHashSecure,
                                                   0,
                                                   userIDSecure,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   false,
                                                   false,
                                                   false,
                                                   kWickrUserVerificationStatusVerified,
                                                   false,
                                                   false,
                                                   0,
                                                   false,
                                                   0,
                                                   QString(),
                                                   false,
                                                   0,
                                                   QString());
        if (!user) {
            m_operation->log_handler->error("cannot find/create user with ID = " + userID);
            m_messagesFailed++;
            return false;
        } else {
            m_wickrUsers.append(user);
        }
    }

    // Process the User Names, if any
    m_userNames.clear();
    for (QString userName : jsonHandler->getUserNames()) {
        QString serverUserName = userName;

        WickrCore::WickrUser *dest = WickrCore::WickrUser::getUserWithAlias(serverUserName);
        if (dest) {
            m_wickrUsers.append(dest);
        } else {
            m_userNames.append(serverUserName);
        }
    }

    // Lets see if users need to be updated first
    sendMessageValidateUserUpdate();
    return true;
}

void WickrIOActionThread::sendMessageValidateUserUpdate()
{
    if (m_wickrUsers.size() == 0) {
        sendMessageValidateUserSearch();
        return;
    }

    QList<WickrCore::WickrUser *> updateUsers;

    // Check if any of the users need to be updated, especially if the signing key is missing
    for (WickrCore::WickrUser *user : m_wickrUsers) {
        if (user->getUserSigningKey().isEmpty())
            updateUsers.append(user);
    }

    if (updateUsers.size() > 0) {
                WickrUserValidateUpdate *u = new WickrUserValidateUpdate(updateUsers,true,true,true,0);
                QObject::connect(u, &WickrUserValidateUpdate::signalRequestCompleted,
                                 this, &WickrIOActionThread::slotValidateUserUpdateDone,
                                 Qt::QueuedConnection);
                WickrCore::WickrRuntime::taskSvcMakeRequest(u);
    } else {
        sendMessageValidateUserSearch();
    }
}

void WickrIOActionThread::slotValidateUserUpdateDone(WickrUserValidateUpdate *context)
{
    QObject::disconnect(context, &WickrUserValidateUpdate::signalRequestCompleted,
                        this, &WickrIOActionThread::slotValidateUserUpdateDone);

    context->deleteLater();
    sendMessageValidateUserSearch();
}

void WickrIOActionThread::sendMessageValidateUserSearch()
{
    // If there are any usernames to process then do so
    if (m_userNames.size() > 0) {
        QString id = m_userNames.at(0);
        m_userNames.removeAt(0);

        WickrUserValidateSearch *c = new WickrUserValidateSearch(WICKR_USERNAME_ALIAS,id,0);
        QObject::connect(c, &WickrUserValidateSearch::signalRequestCompleted,
                         this, &WickrIOActionThread::slotValidateUserCheckDone,
                         Qt::QueuedConnection);
        if (!WickrCore::WickrRuntime::taskSvcMakeRequest(c)) {
            qDebug() << "sendMessageValidateUser: taskSvcMakeRequest failed!";
            setProcessAction(false);
        }

        qDebug() << "searching for " << id;
    } else {
        emit signalSendMessageDoneGetUsers();
    }
}

void WickrIOActionThread::slotValidateUserCheckDone(WickrUserValidateSearch *context)
{
    QObject::disconnect(context, &WickrUserValidateSearch::signalRequestCompleted,
                        this, &WickrIOActionThread::slotValidateUserCheckDone);

    if (context->isSuccess()) {

        const QList<WickrCore::WickrUserValidatorResult>& theResults = context->validationResults();
        if (theResults.size() != 0) {
            for( int i=0; i<theResults.size(); i++ ) {
                WickrCore::WickrUserValidatorResult oneResult = theResults.at(i);

                WickrCore::WickrUser *oneUser = oneResult.getUser();
                if (oneUser != nullptr) {
                    m_wickrUsers.append(oneUser);
                }
                sendMessageValidateUserSearch();
            }
        } else {
            setProcessAction(false);
        }
    } else {
        setProcessAction(false);
    }

    // Free the JSON Handler object
    if (!processActionState() && m_jsonHandler != NULL) {
        delete m_jsonHandler;
        m_jsonHandler = NULL;
    }
    context->deleteLater();
}

/**
 * @brief WickrIOActionThread::slotSendMessagePostGetUsers
 * This slot will be called when the signalSendMessageDoneGetUsers signal is emmitted.
 * This will be after all of the users for this message have been successfully
 * retrieved.
 */
void WickrIOActionThread::slotSendMessagePostGetUsers()
{
    /*
     * Create the Convo
     */
    WickrCore::WickrConvo          *convo;
    WickrConvoInfoType      convoType = m_wickrUsers.size() > 1 ? CONVO_SECURE_ROOM : CONVO_ONE_TO_ONE;

    // get the gid for this set of users
    QString vGid = WickrCore::WickrConvo::getVGroupIDWithUsers(m_wickrUsers, convoType );

    convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGid);
    if ( convo == 0 ) {
        convo = new WickrCore::WickrConvo(vGid, nullptr, m_wickrUsers, convoType);

        if (m_wickrUsers.size() > 1) {
            convo->setVGroupTag("");
        }
    }

    bool sentSuccess;
    if (convo->getConvoType() == CONVO_ONE_TO_ONE) {
        sentSuccess = sendMessageTo1To1(convo);
    } else {
        sentSuccess = sendMessageToConvo(convo);
    }

    // If there was a problems sending then clean up
    if (!sentSuccess) {
        // Free the JSON Handler object
        delete m_jsonHandler;
        m_jsonHandler = NULL;

        setProcessAction(false);

        emit signalStartProcessDatabase(m_curActionID, false);
    }
}

bool
WickrIOActionThread::sendMessageToConvo(WickrCore::WickrConvo *convo)
{
    if (!convo || convo->getAllUsers().length() == 0) {
        m_operation->log_handler->error("Convo is not set or no users in convo!");
        m_messagesFailed++;
        return false;
    }

    /*
     * Setup and send the message
     */
#if 0
    QList<WickrCore::WickrAttachment> attachments;

    // TODO: Handle the attachment
    QList<QString> attachmentFiles = m_jsonHandler->getAttachments();
    if (attachmentFiles.size() > 0) {
        for (QString attachmentFile : attachmentFiles) {
            QFile att(attachmentFile);
            if( att.exists() ) {
                att.open( QFile::ReadOnly );
                QByteArray contents = att.readAll();
                att.close();

                WickrCore::WickrAttachment a = WickrCore::WickrAttachment(contents);
                attachments.append(a);
            } else {
                m_operation->log_handler->error("Cannot open attachment file: "+attachmentFile);
            }
        }
    }
#endif

    long ttl = m_jsonHandler->getTTL();
    if (ttl == 0) {
        ttl = convo->getDestruct();
    } else {
        if (ttl != convo->getDestruct())
            convo->setDestruct(ttl);
    }

    if (m_jsonHandler->hasBOR()) {
    long bor = m_jsonHandler->getBOR();
        convo->setBOR(bor);
    }

#if 0
    if (attachmentFiles.size() > 0) {
        if (!sendFile(convo, attachmentFiles, m_jsonHandler->getMessage())) {
            setProcessAction(false);
        }
    } else {
#endif
        // Get the users
        QList<WickrCore::WickrUser *> userList;

        QList<WickrCore::WickrUser *> convoUsers = convo->getAllUsers();
        WickrCore::WickrUser *selfUser = WickrCore::WickrUser::getSelfUser();
        for (WickrCore::WickrUser *user : convoUsers) {
            if (user != selfUser) {
                userList.append(user);
            }
        }
        // Send message
        WickrSendContext *context = new WickrSendContext(MsgType_Text,
                                                         convo,
                                                         WickrCore::WickrMessage::createTextMsgBody(m_jsonHandler->getMessage(),convo),
                                                         userList);
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionThread::slotMessageDone, Qt::QueuedConnection);
        WickrCore::WickrRuntime::msgSvcSend(context);
#if 0
   }
#endif

    // Free the JSON Handler object
    delete m_jsonHandler;
    m_jsonHandler = NULL;
    return true;
}

bool WickrIOActionThread::sendFile(WickrCore::WickrConvo *targetConvo, const QList<QString> files, const QString& comments)
{
    if (files.size() == 0)
        return false;

    QString name = files.at(0); // Orig file name
    QString finalFileName = QFileInfo(name).fileName();

    WickrCore::FetchInformation fetchInfo;
    QByteArray encryptionKeyAES = convertCFDataToByteArray( ::randomGCMKey(), false );

    QUuid uuid = QUuid::createUuid();
    fetchInfo.guid = uuid.toString().mid(1,36).toUpper();
    fetchInfo.segment = 1;
    fetchInfo.key = encryptionKeyAES;
    //qDebug() << "guid:" << fetchInfo.guid << "key:" << encryptionKeyAES.toHex();

    QMimeDatabase db;
    QUrl fileAsUrl = QUrl::fromLocalFile(name);
    QString metaDataMimeType = db.mimeTypeForUrl(fileAsUrl).name();
    qint64 fileSize = 0;

    QFileInfo primaryFile(name);
    if(primaryFile.exists())
    {
        fileSize = primaryFile.size();
    }

    QList <WickrCore::FetchInformation> fetchInfoList;
    fetchInfoList.append(fetchInfo);
    QString fileNameBeforeEncryption = name;
    QString fileNameAfterEncryption = WickrAppContext::getAttachmentsDir().absoluteFilePath(fetchInfo.guid);

    // ENCRYPT FILE HERE
    // This copy operation should be replaced by encryption function that saves to the file

    QFutureWatcher<QString> * encryptionWatcher = new QFutureWatcher<QString>();
    connect(encryptionWatcher, &QFutureWatcher<QString>::finished, this, [=] {

        qDebug() << "file upload encryption finished";
        QString hashResult(encryptionWatcher->result());
        if ( hashResult.isEmpty() ) {
            qDebug() << "WickrIOActionThread::sendFile: encryptFile failed, copying encrypted file!!";
            QFile::copy( fileNameBeforeEncryption, fileNameAfterEncryption );
        }

        //qDebug() << metaDataMimeType << fileNameBeforeEncryption;
        if (metaDataMimeType == "image/png" || metaDataMimeType == "image/jpeg" || metaDataMimeType == "image/bmp" || metaDataMimeType == "image/gif")
        {
            QImage orig(fileNameBeforeEncryption);
            if(!orig.isNull())
            {
                WickrCore::Dimension fileDim;
                fileDim.height = orig.height();
                fileDim.width = orig.width();
                QString resourceName = QUuid::createUuid().toString().mid(1,36).toUpper();

//                imageServer->addImage(resourceName, orig);
                WickrCore::FileMetaData imageFileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, fileDim, comments);
                WickrCore::FileInfo fileToUpload(finalFileName, imageFileMetaData);
                WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
            else
            {
                qDebug() << "can't seem to read image";
                WickrCore::FileMetaData fileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, comments);
                WickrCore::FileInfo fileToUpload(finalFileName, fileMetaData);
                WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
        }
        else
        {
            WickrCore::FileMetaData fileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, comments);
            WickrCore::FileInfo fileToUpload(finalFileName, fileMetaData);
            WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
        }
        encryptionWatcher->deleteLater();
    });
    QFuture<QString> encryptionStep = QtConcurrent::run(&WickrFileTransferService::encryptFile, encryptionKeyAES, fileNameBeforeEncryption, fileNameAfterEncryption);
    encryptionWatcher->setFuture(encryptionStep);
    return true;
}

#include "wickrIOClientRuntime.h"
void
WickrIOActionThread::cleanup(const QString& uuid)
{
    if (!uuid.isEmpty() && WickrIOClientRuntime::getFileSendCleanup()) {
        // Perform some cleanup, remove the encrypted file
        QString fileNameAfterEncryption = WickrAppContext::getAttachmentsDir().absoluteFilePath(uuid);
        QFile encFile(fileNameAfterEncryption);
        if (encFile.exists()) {
            if (!encFile.remove()) {
                qDebug() << "Cannot remove sent encrypted file:" << uuid;
            }
        }
    }
}

void
WickrIOActionThread::slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName)
{
    Q_UNUSED(progress);
    Q_UNUSED(finalFileName);

    qDebug() << "File status changed";
    qDebug() << "UUID=" << uuid;
    qDebug() << "Status=" << status;

    if (status == "complete") {
        setProcessAction(false);
        cleanup(uuid);
        emit signalStartProcessDatabase(m_curActionID, true);
    } else if (status == "uploadinterrupted" ||
               status == "canceled") {
        setProcessAction(false);
        cleanup(uuid);
        emit signalStartProcessDatabase(m_curActionID, false);
    } else {
        qDebug() << "not signaling signalStartProcessDatabase()";
    }
}

/**
 * @brief WickrIOActionThread::msgSendDone
 * This SLOT is called when a message send is completed.
 * @param ws Pointer to the WickrMsgsendService object
 */

//TODO: No way to get indication of successful message send???
void WickrIOActionThread::slotMessageDone(WickrSendContext *context)
{
    if (context->isSuccess()) {
        m_messagesSent++;
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_MSGS_TX, DB_STATDESC_TOTAL, 1);
        }
#if 1
        m_appCounter.add(1);
#else
        m_appCounter.add(ms->wickrBotMsgNumApps);
#endif

        // Do not need to keep the outbox message around, so free it
#if 0 //TODO: HOW DO WE FREE THE OUTBOX MESSAGE
        WickrCore::WickrOutbox *outMsg = ms->getOutboxMessage();
        outMsg->dodelete();
#endif
        context->deleteLater();
        emit signalStartProcessDatabase(m_curActionID, true);
    } else {
        qDebug() << "IN slotMessageDone(): failed";
//        m_operation->log_handler->error(QString("Message send error: " + context->getResult()));
        m_messagesFailed++;
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_ERRORS_TX, DB_STATDESC_TOTAL, 1);
        }
//        qDebug() << "The Message was NOT sent successfully" << ms->getResult();

        bool deleteMsg = true;

#if 0
        WickrCore::WickrOutbox *outMsg = ms->getOutboxMessage();
        QList<WickrCore::WickrError *> errors = outMsg->getErrors();
        if (errors.length() > 0) {
            WickrCore::WickrError *error = errors.at(0);
            if (error->getErrCode() == SIEM_RATE_LIMIT) {
                // TODO: Need to delay sending more messages!!!
                // TODO: Do not delete the message to be sent!!!
                deleteMsg = false;
                m_backoff = 30;
            }
        }

        // Do not need to keep the outbox message around, so free it
        if (deleteMsg) {
            outMsg->dodelete();
        }
#endif

        context->deleteLater();

        setProcessAction(false);

        // TODO: Depending on the error do different things
        if (deleteMsg) {
            emit signalStartProcessDatabase(m_curActionID, false);
        } else {
            emit signalStartProcessDatabase(WICKRIO_AH_INVALID_ID, false);
        }
    }
}


/**
 * @brief WickrIOActionThread::cleanUpDatabase
 * This function should be called when shutting down to clean up the database.
 * Cleanup includes removing all known conversations.
 */
void WickrIOActionThread::cleanUpDatabase()
{
//    cleanUpConvoList();
}

/**
 * @brief WickrIOActionThread::cleanUpConvoList
 * This function will delete all of the convos.
 * @param ws The WickrConvolistService
 */
void WickrIOActionThread::cleanUpConvoList()
{
    WickrConvoList *convos = WickrCore::WickrConvo::getConvoList();
    if (convos != NULL && convos->size() > 0) {
        qDebug() << "CleanUpConvoList: number of convos to delete=" << convos->size();
        QList<QString> keys = convos->acquireKeyList();

        // Process each of the convos
        for (int i=0; i < keys.size(); i++) {
            WickrCore::WickrConvo *currentConvo = (WickrCore::WickrConvo *)convos->value( keys.at(i) );
            // Remove the convo
            currentConvo->dodelete(WickrCore::WickrConvo::DeleteInternal, false);
        }
    }
}

#include "createjson.h"

void WickrIOActionThread::sendStatusMessage(WickrBotJson *jsonHandler, bool success)
{
    // Create the message to send
    QString message2send;
    QString successMsg = success ? "successfully" : "NOT successfully";
    if (jsonHandler->getAttachments().length() > 0) {
        message2send = QString("File (%1) was %2 sent to %3").arg(jsonHandler->getAttachments()[0])
                .arg(successMsg)
                .arg(jsonHandler->getUserNames().join(","));
    } else {
        message2send = QString("Message was %2 sent to %3")
                .arg(successMsg)
                .arg(jsonHandler->getUserNames().join(","));
    }

    // Create the json to initiate the sending of this message
    QStringList users;
    users.append(jsonHandler->getStatusUser());
    CreateJsonAction action("sendmessage",
                            users,
                            0,
                            message2send,
                            QList<QString>(),
                            QString());

    QByteArray json = action.toByteArray();
    QDateTime dt = QDateTime::currentDateTime();

    m_operation->m_botDB->insertAction(json, dt, m_operation->m_client->id);
}

/**
 * @brief WickrIOMain::processDatabase
 * This function is called to start the processing of the next action from the
 * database. The input action ID will be removed from the database, a value of
 * WICKRBOT_INVALID_ID will be used if there is no previous action to remove.
 * This function SHOULD only be called when an action has completed.  Actions
 * must be performed in order.
 * @param deleteID Id of the action to delete, or WICKRBOT_INALID_ID
 */
void WickrIOActionThread::processDatabase(int deleteID, bool success)
{
    if (m_operation->m_botDB == NULL) {
        m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);
    }

    // If it is time to output stats then do so
    if (m_outputStats) {
        this->logCounts();
        m_outputStats = false;
    }

    /*
     * If the input ID is valid then delete that entry from the database
     */
    if (deleteID != WICKRIO_AH_INVALID_ID) {
        WickrBotActionCache action;

        if (m_operation->m_botDB->getAction(deleteID, &action)) {
            WickrBotJson jsonHandler;
            if (jsonHandler.parseJsonString(action.json)) {
                // If there is a status user then send a message to that user
                if (!jsonHandler.getStatusUser().isEmpty()) {
                    sendStatusMessage(&jsonHandler, success);
                }
            }
        }
        m_operation->m_botDB->deleteAction(deleteID);
    }

    /*
     * If we are about to shutdown then stop processing actions
     */
    if (m_shuttingdown || !m_running) {
        if (processActionState()) {
            setProcessAction(false);
        }
        return;
    }

    int curCntAppsSentRecently = m_appCounter.getCumulative(60);
    if (curCntAppsSentRecently >= 2000) {
        qDebug() << "Cummulative apps sent to in last 60 seconds is " << curCntAppsSentRecently;
        qDebug() << "Is greater than or equal to 180 so we will wait till it is less";
        setProcessAction(false);
        m_backoff = 65;
        return;
    }

    /*
     * Process the actions from the databse
     */
    WickrBotActionCache *action = new WickrBotActionCache();
    bool gotAction;

    QDateTime dt = QDateTime::currentDateTime();
    QString dtString = dt.toString(DB_DATETIME_FORMAT);

    if (m_operation->m_client != NULL) {
        gotAction = WickrBotDatabase::getFirstAction(m_operation->m_botDB, dtString, action, m_operation->m_client->id);
    } else {
        gotAction = WickrBotDatabase::getFirstAction(m_operation->m_botDB, dtString, action);
    }
    if (gotAction) {
        WickrBotJson *jsonHandler = new WickrBotJson();
        bool doneWithAction = false;

        if (jsonHandler->parseJsonString(action->json)) {
            // Action is starting, set flag so no other action is performed
            setProcessAction(true);

            if (jsonHandler->getAction() == WickrBotJson::ACTION_SEND_MESSAGE) {
                if (!processActionSendMessage(jsonHandler, action->id)) {
                    setProcessAction(false);
                    doneWithAction = true;
                }
            } else {
                setProcessAction(false);
                doneWithAction = true;
            }
        } else {
            doneWithAction = true;
        }

        // If done with the action then delete it from the database
        if (doneWithAction) {
            delete jsonHandler;
            m_operation->m_botDB->deleteAction(action->id);
        }
    } else {
        setProcessAction(false);
    }
    delete action;
}
