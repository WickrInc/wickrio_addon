#include <QUuid>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include "wickrIOActionHdlr.h"
#include "Wickr/aes/AESHelper.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "wickrbotactiondatabase.h"

WickrIOActionHdlr::WickrIOActionHdlr(OperationData *operation) :
    m_operation(operation),
    m_processAction(false),
    m_processCleanUp(false),
    m_delayedRcvOrClean(false),
    m_backoff(0),
    m_timerStatsTicker(0),
    m_outputStats(false),
    m_shutdownTime(false),
    m_shuttingdown(false)
{
    initCounts();
    m_appCounter.clear();

    this->connect(this, &WickrIOActionHdlr::signalSendMessageDoneGetUsers, this, &WickrIOActionHdlr::slotSendMessagePostGetUsers);
    this->connect(this, &WickrIOActionHdlr::signalStartProcessDatabase, this, &WickrIOActionHdlr::processDatabase);

    WickrFileTransferService *ftSvc = WickrCore::WickrRuntime::ftSvc();
    if (ftSvc) {
        connect(ftSvc, &WickrFileTransferService::statusChanged, this, &WickrIOActionHdlr::slotSendFileStatusChange);
    } else {
        qDebug() << "WickrIOActionHdlr: cannot get file transfer service!";
    }
}

WickrIOActionHdlr::~WickrIOActionHdlr()
{
}

bool
WickrIOActionHdlr::sendMessageTo1To1(WickrCore::WickrConvo *convo)
{
    // Make sure there are actually users in this 1to1
    if (!convo || convo->getAllUsers().length() == 0) {
        qDebug() << "sendMessageTo1To1: no users in 1to1!";
        return false;
    }

    /*
     * Setup and send the message
     */
//    QList<WickrCore::WickrAttachment> attachments;

    // TODO: Handle the attachment
    QList<QString> attachmentFiles = m_jsonHandler->getAttachments();
    if (attachmentFiles.size() > 0) {
        for (QString attachmentFile : attachmentFiles) {
            QFile att(attachmentFile);
            if( att.exists() ) {
                att.open( QFile::ReadOnly );
                QByteArray contents = att.readAll();
                att.close();

//                WickrCore::WickrAttachment a = WickrCore::WickrAttachment(contents);
//                attachments.append(a);
            } else {
                m_operation->log_handler->error("Cannot open attachment file: "+attachmentFile);
            }
        }
    }

    long ttl = m_jsonHandler->getTTL();
    if (ttl == 0) {
        ttl = convo->getDestruct();
    } else {
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
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionHdlr::slotMessageDone, Qt::QueuedConnection);
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
 * @brief WickrIOActionHdlr::processActionSendMessage
 * This function will handle the Send Message action. The input jsonHandler argument will
 * contain all of the necessary information to identify who to send the message to and
 * the actual message.
 * @param jsonHandler Details of the message to be sent
 * @param actionID The ID of the action from the database. Need to remove after success.
 */
bool WickrIOActionHdlr::processActionSendMessage(WickrBotJson *jsonHandler, int actionID)
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
                                                   0);
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
//        QString serverUserName = WickrIOEClientMain::serverId(userName);
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

void WickrIOActionHdlr::sendMessageValidateUserUpdate()
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
                                 this, &WickrIOActionHdlr::slotValidateUserUpdateDone,
                                 Qt::QueuedConnection);
                WickrCore::WickrRuntime::taskSvcMakeRequest(u);
    } else {
        sendMessageValidateUserSearch();
    }
}

void WickrIOActionHdlr::slotValidateUserUpdateDone(WickrUserValidateUpdate *context)
{
    QObject::disconnect(context, &WickrUserValidateUpdate::signalRequestCompleted,
                        this, &WickrIOActionHdlr::slotValidateUserUpdateDone);

    context->deleteLater();
    sendMessageValidateUserSearch();
}

void WickrIOActionHdlr::sendMessageValidateUserSearch()
{
    // If there are any usernames to process then do so
    if (m_userNames.size() > 0) {
        QString id = m_userNames.at(0);
        m_userNames.removeAt(0);

        WickrUserValidateSearch *c = new WickrUserValidateSearch(WICKR_USERNAME_ALIAS,id,0);
        QObject::connect(c, &WickrUserValidateSearch::signalRequestCompleted,
                         this, &WickrIOActionHdlr::slotValidateUserCheckDone,
                         Qt::QueuedConnection);
        WickrCore::WickrRuntime::taskSvcMakeRequest(c);

        qDebug() << "searching for " << id;
    } else {
        emit signalSendMessageDoneGetUsers();
    }
}

void WickrIOActionHdlr::slotValidateUserCheckDone(WickrUserValidateSearch *context)
{
    QObject::disconnect(context, &WickrUserValidateSearch::signalRequestCompleted,
                        this, &WickrIOActionHdlr::slotValidateUserCheckDone);

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
            m_processAction = false;
        }
    } else {
        m_processAction = false;
    }

    // Free the JSON Handler object
    if (!m_processAction && m_jsonHandler != NULL) {
        delete m_jsonHandler;
        m_jsonHandler = NULL;
    }
    context->deleteLater();
}

/**
 * @brief WickrIOActionHdlr::slotSendMessagePostGetUsers
 * This slot will be called when the signalSendMessageDoneGetUsers signal is emmitted.
 * This will be after all of the users for this message have been successfully
 * retrieved.
 */
void WickrIOActionHdlr::slotSendMessagePostGetUsers()
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

        m_processAction = false;

        emit signalStartProcessDatabase(m_curActionID);
    }
}

bool
WickrIOActionHdlr::sendMessageToConvo(WickrCore::WickrConvo *convo)
{
    if (!convo) {
        m_operation->log_handler->error("Convo is not set!");
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
            m_processAction = false;
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
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionHdlr::slotMessageDone, Qt::QueuedConnection);
        WickrCore::WickrRuntime::msgSvcSend(context);
#if 0
   }
#endif

    // Free the JSON Handler object
    delete m_jsonHandler;
    m_jsonHandler = NULL;
    return true;
}

bool WickrIOActionHdlr::sendFile(WickrCore::WickrConvo *targetConvo, const QList<QString> files, const QString& comments)
{
    if (files.size() == 0)
        return false;

    QString name = files.at(0);

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

    QString fileNameAfterEncryption = WickrAppContext::getFilesDir() + fetchInfo.guid;

    // ENCRYPT FILE HERE
    // This copy operation should be replaced by encryption function that saves to the file

    QFutureWatcher<QString> * encryptionWatcher = new QFutureWatcher<QString>();
    connect(encryptionWatcher, &QFutureWatcher<QString>::finished, this, [=] {

        qDebug() << "file upload encryption finished";
        QString hashResult(encryptionWatcher->result());
        if ( hashResult.isEmpty() ) {
            qDebug() << "WickrIOActionHdlr::sendFile: encryptFile failed, copying encrypted file!!";
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
                WickrCore::FileMetaData imageFileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, comments);
                WickrCore::FileInfo fileToUpload(name, imageFileMetaData);
                WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
            else
            {
                qDebug() << "can't seem to read image";
                WickrCore::FileMetaData fileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, comments);
                WickrCore::FileInfo fileToUpload(name, fileMetaData);
                WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
        }
        else
        {
            WickrCore::FileMetaData fileMetaData(metaDataMimeType, fileSize, fetchInfoList, hashResult, comments);
            WickrCore::FileInfo fileToUpload(name, fileMetaData);
            WickrCore::WickrRuntime::ftScheduleUpload(targetConvo, fileNameAfterEncryption, fileToUpload);
        }
        encryptionWatcher->deleteLater();
    });
    QFuture<QString> encryptionStep = QtConcurrent::run(&WickrFileTransferService::encryptFile, encryptionKeyAES, fileNameBeforeEncryption, fileNameAfterEncryption);
    encryptionWatcher->setFuture(encryptionStep);
    return true;
}

void
WickrIOActionHdlr::slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName)
{
    Q_UNUSED(progress);
    Q_UNUSED(finalFileName);

    qDebug() << "File status changed";
    qDebug() << "UUID=" << uuid;
    qDebug() << "Status=" << status;

    if (status == "complete") {
        m_processAction = false;
        emit signalStartProcessDatabase(m_curActionID);
    } else if (status == "uploadinterrupted" ||
               status == "canceled") {
        m_processAction = false;
        emit signalStartProcessDatabase(m_curActionID);
    } else {
        qDebug() << "not signaling signalStartProcessDatabase()";
    }
}

/**
 * @brief WickrIOActionHdlr::msgSendDone
 * This SLOT is called when a message send is completed.
 * @param ws Pointer to the WickrMsgsendService object
 */

//TODO: No way to get indication of successful message send???
void WickrIOActionHdlr::slotMessageDone(WickrSendContext *context)
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
        qDebug() << "The Message was sent successfully";

        // Do not need to keep the outbox message around, so free it
#if 0 //TODO: HOW DO WE FREE THE OUTBOX MESSAGE
        WickrCore::WickrOutbox *outMsg = ms->getOutboxMessage();
        outMsg->dodelete();
#endif
        context->deleteLater();
        emit signalStartProcessDatabase(m_curActionID);
    } else {
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

        m_processAction = false;

        // TODO: Depending on the error do different things
        if (deleteMsg) {
            emit signalStartProcessDatabase(m_curActionID);
        } else {
            emit signalStartProcessDatabase(WICKRBOT_INVALID_ID);
        }
    }
}


/**
 * @brief WickrIOActionHdlr::cleanUpDatabase
 * This function should be called when shutting down to clean up the database.
 * Cleanup includes removing all known conversations.
 */
void WickrIOActionHdlr::cleanUpDatabase()
{
//    cleanUpConvoList();
}

/**
 * @brief WickrIOActionHdlr::cleanUpConvoList
 * This function will delete all of the convos.
 * @param ws The WickrConvolistService
 */
void WickrIOActionHdlr::cleanUpConvoList()
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


/**
 * @brief WickrIOActionHdlr::doTimerWork
 * This method will perform operations that are to be done when our one second timer
 * goes off. The main signal is to initiate a Process Database operation.
 */
void WickrIOActionHdlr::doTimerWork()
{
    // Increment the App Counter, needed to backup the number of Apps sent to
    m_appCounter.incSecond();
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRBOT_UPDATE_STATS_SECS) == 0) {
        m_outputStats = true;
//        QMetaObject::invokeMethod(m_rxThread, "logCounts", Qt::QueuedConnection);
    }

    // Only print out if app count is greater than 0 (reduce output)
    int appCount = m_appCounter.getCumulative(60);
    if (appCount) {
        qDebug() << "Current app count for the last minute=" << appCount;
    }

    if (! m_processCleanUp) {
        if ((m_timerStatsTicker % m_operation->cleanUpSecs) == 0 || m_delayedRcvOrClean) {
            // If actions are bein processed then delay the db clean
            if (m_processAction) {
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
    else if (! m_processAction) {
        if (! m_processCleanUp)
            emit signalStartProcessDatabase(WICKRBOT_INVALID_ID);
    }

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
void WickrIOActionHdlr::processDatabase(int deleteID)
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
    if (deleteID != WICKRBOT_INVALID_ID) {
        m_operation->m_botDB->deleteAction(deleteID);
    }

    /*
     * If we are about to shutdown then stop processing actions
     */
    if (m_shutdownTime) {
        m_processAction = false;
        return;
    }

    int curCntAppsSentRecently = m_appCounter.getCumulative(60);
    if (curCntAppsSentRecently >= 2000) {
        qDebug() << "Cummulative apps sent to in last 60 seconds is " << curCntAppsSentRecently;
        qDebug() << "Is greater than or equal to 180 so we will wait till it is less";
        m_processAction = false;
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

        if (jsonHandler->parseJsonString(action->json)) {

            // Action is starting, set flag so no other action is performed
            m_processAction = true;
            bool doneWithAction = false;

            if (jsonHandler->getAction() == WickrBotJson::ACTION_SEND_MESSAGE) {
                if (!processActionSendMessage(jsonHandler, action->id)) {
                    m_processAction = false;
                    doneWithAction = true;
                }
            } else {
                m_processAction = false;
                doneWithAction = true;
            }

            // If done with the action then delete it from the database
            if (doneWithAction) {
                delete jsonHandler;
                m_operation->m_botDB->deleteAction(action->id);
            }
        }
    } else {
        m_processAction = false;
    }
    delete action;
}
