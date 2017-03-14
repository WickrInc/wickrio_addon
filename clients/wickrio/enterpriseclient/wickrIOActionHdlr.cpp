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

    WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
    if (cloudMgr) {
        connect(cloudMgr, &WickrCore::WickrCloudTransferMgr::statusChanged, this, &WickrIOActionHdlr::slotSendFileStatusChange);
    }
}

WickrIOActionHdlr::~WickrIOActionHdlr()
{
}

/**
 * @brief WickrIOActionHdlr::processAction
 * This function will determine what action is to be processed and call the appropriate
 * function to perform the action.  The default action is to send a message.  The
 * m_processAction boolean is set to true so that no other action should be performed
 * while the current action is being handled.
 * @param jsonHandler The JSON Handler, that contains all of the action information
 * @param actionID The ID of the action from the database. Need to remove after success.
 */
void WickrIOActionHdlr::processAction(WickrBotJson *jsonHandler, int actionID)
{
    // Action is starting, set flag so no other action is performed
    m_processAction = true;

    if (jsonHandler->getAction() == WickrBotJson::ACTION_SEND_MESSAGE) {
        if (!processActionSendMessage(jsonHandler, actionID)) {
            m_processAction = false;
            delete jsonHandler;
        }
    } else {
        m_processAction = false;
    }
}

void
WickrIOActionHdlr::sendMessageTo1To1(WickrCore::WickrConvo *convo)
{
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
                m_operation->error("Cannot open attachment file: "+attachmentFile);
            }
        }
    }

    if (attachmentFiles.size() > 0) {
        if (! sendFile(convo, attachmentFiles, m_jsonHandler->getMessage())) {
            m_processAction = false;
        }
    } else {
#endif
        // Send message
        WickrSendContext *context = new WickrSendContext(MsgType_Text, convo, WickrCore::WickrMessage::createTextMsgBody(m_jsonHandler->getMessage(),convo));
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionHdlr::slotMessageDone, Qt::QueuedConnection);
        WickrCore::WickrRuntime::msgSvcSend(context);
#if 0
    }
#endif

    // Free the JSON Handler object
    delete m_jsonHandler;
    m_jsonHandler = NULL;
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
            m_operation->error("Send message has no users or vgroupid");
            return false;
        }

        WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
        if ( convo == 0 ) {
            m_operation->error("cannot convo with vgroupid = " + vGroupID);
            m_messagesFailed++;
            return false;
        }
        if (convo->getConvoType() == CONVO_ONE_TO_ONE) {

        } else {
            sendMessageToConvo(convo);
        }
        return true;
    }

    for (QString userID : jsonHandler->getUserIDs()) {
        QByteArray userIDSecure;
        WickrStatus status(0);

        userIDSecure = encryptUserDataString(userID, status);

        if (status.isError()) {
            m_operation->error("Error encrypting user data!");
            m_messagesFailed++;
            return false;
        }


        WickrCore::WickrUser *user;
        user = WickrCore::WickrUser::getUserWithID(userID,
                                                   0,
                                                   userIDSecure,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   0,
                                                   false,
                                                   false,
                                                   kWickrUserVerificationStatusVerified,
                                                   false,
                                                   0,
                                                   false,
                                                   0,
                                                   false);
        if (!user) {
            m_operation->error("cannot find/create user with ID = " + userID);
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

    sendMessageValidateUser();
    return true;
}

void WickrIOActionHdlr::sendMessageValidateUser()
{
    // If there are any usernames to process then do so
    if (m_userNames.size() > 0) {
        QString id = m_userNames.at(0);
        m_userNames.removeAt(0);

        WickrUserValidateSearch *c = new WickrUserValidateSearch(WICKR_USERNAME_ALIAS,id,0);
        QObject::connect(c, &WickrUserValidateSearch::signalRequestCompleted,
                         this, &WickrIOActionHdlr::slotUserValidated,Qt::QueuedConnection);
        WickrCore::WickrRuntime::taskSvcMakeRequest(c);

        qDebug() << "searching for " << id;
    } else {
        emit signalSendMessageDoneGetUsers();
    }
}

void WickrIOActionHdlr::slotUserValidated(WickrUserValidateSearch *context)
{
    if (context->isSuccess()) {

        const QList<WickrCore::WickrUserValidatorResult>& theResults = context->validationResults();
        if (theResults.size() != 0) {
            for( int i=0; i<theResults.size(); i++ ) {
                WickrCore::WickrUserValidatorResult oneResult = theResults.at(i);

                WickrCore::WickrUser *oneUser = oneResult.getUser();
                if (oneUser != nullptr) {
                    m_wickrUsers.append(oneUser);
                }
                sendMessageValidateUser();
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

    // get the gid for this set of users
    QString vGid = WickrCore::WickrConvo::getVGroupIDWithUsers(m_wickrUsers);

    convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGid);
    if ( convo == 0 ) {
        convo = new WickrCore::WickrConvo(vGid, "", m_wickrUsers);

        if (m_wickrUsers.size() > 1) {
            convo->setVGroupTag("");
        }
    }
    if (convo->getConvoType() == CONVO_ONE_TO_ONE) {
        sendMessageTo1To1(convo);
    } else {
        sendMessageToConvo(convo);
    }
}

void WickrIOActionHdlr::sendMessageToConvo(WickrCore::WickrConvo *convo)
{
    if (!convo) {
        m_operation->error("Convo is not set!");
        m_processAction = false;
        m_messagesFailed++;

        // Free the JSON Handler object
        delete m_jsonHandler;
        m_jsonHandler = NULL;

        return;
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
                m_operation->error("Cannot open attachment file: "+attachmentFile);
            }
        }
    }
#endif

    long ttl = m_jsonHandler->getTTL();
    if (ttl == 0) {
        ttl = convo->getDestruct();
    }

#if 0
    if (attachmentFiles.size() > 0) {
        if (!sendFile(convo, attachmentFiles, m_jsonHandler->getMessage())) {
            m_processAction = false;
        }
    } else {
#endif
        // Send message
        WickrSendContext *context = new WickrSendContext(MsgType_Text, convo, WickrCore::WickrMessage::createTextMsgBody(m_jsonHandler->getMessage(),convo));
        connect(context, &WickrSendContext::signalRequestCompleted, this, &WickrIOActionHdlr::slotMessageDone, Qt::QueuedConnection);
        WickrCore::WickrRuntime::msgSvcSend(context);
#if 0
   }
#endif

    // Free the JSON Handler object
    delete m_jsonHandler;
    m_jsonHandler = NULL;
}

bool WickrIOActionHdlr::sendFile(WickrCore::WickrConvo *targetConvo, const QList<QString> files, const QString& comments)
{
    if (files.size() == 0)
        return false;

    QString name = files.at(0);

    WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
    if (! cloudMgr) {
        return false;
    }

    WickrCore::FetchInformation fetchInfo;

    QByteArray encryptionKeyAES = convertCFDataToByteArray( ::randomLocalKey(), false );

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

    // construct FileMetaData
    WickrCore::FileMetaData fileMetaData(metaDataMimeType, fileSize, fetchInfoList, comments);

    QString fileNameBeforeEncryption = name;

    QString fileNameAfterEncryption = WickrAppContext::getFilesDir() + fetchInfo.guid;

    // ENCRYPT FILE HERE
    // This copy operation should be replaced by encryption function that saves to the file

    QFutureWatcher<QString> * encryptionWatcher = new QFutureWatcher<QString>();
    connect(encryptionWatcher, &QFutureWatcher<QString>::finished, this, [=] {

        qDebug() << "file upload encryption finished";
        QString imageHash(encryptionWatcher->result());
        if ( imageHash.isEmpty() ) {
            qDebug() << "WickrIOActionHdlr::sendFile: encryptFile failed, copying encrypted file!!";
            QFile::copy( fileNameBeforeEncryption, fileNameAfterEncryption );
        }

        //qDebug() << metaDataMimeType << fileNameBeforeEncryption;
        if (metaDataMimeType == "image/png" || metaDataMimeType == "image/jpeg" || metaDataMimeType == "image/bmp" || metaDataMimeType == "image/gif")
        {
            QImage orig(fileNameBeforeEncryption);
            if(!orig.isNull()) {
                int maxPreviewHeight = 1000;
                int maxPreviewWidth = 1000;
                int maxThumbHeight = 100;
                int maxThumbWidth = 100;

                WickrCore::Dimension fileDim;
                fileDim.height = orig.height();
                fileDim.width = orig.width();

                // create the preview size from the original

                QImage result(orig);

                if(orig.height() > maxPreviewHeight || orig.width() > maxPreviewWidth)
                {
                    result = orig.scaledToHeight(maxPreviewWidth).scaledToHeight(maxPreviewHeight, Qt::SmoothTransformation);
                    if(result.width() > maxPreviewWidth)
                    {
                        result = orig.scaledToWidth(maxPreviewWidth, Qt::SmoothTransformation);
                    }
                }

                QString resourceName = QUuid::createUuid().toString().mid(1,36).toUpper();

                QByteArray previewKeyAES = convertCFDataToByteArray( ::randomLocalKey(), false );
                QString previewFileNameAfterEncryption( WickrAppContext::getFilesDir() + resourceName );
                QString previewFileNameBeforeEncryption( previewFileNameAfterEncryption + "_tmp" );

                //qDebug () << "saving preview image as " << previewFileNameBeforeEncryption;
                result.save(previewFileNameBeforeEncryption, "PNG");

                // ENCRYPT PREVIEW FILE HERE
                // Disk encryption can overwrite the output of result.save
                //  -or-
                // Memory encryption can get the file contents of result, encrypt it, and save
                QString previewHash = cloudMgr->encryptFile( previewKeyAES, previewFileNameBeforeEncryption, previewFileNameAfterEncryption );
                if ( previewHash.isEmpty() ) {
                    qDebug() << "WickrIOActionHdlr::sendFile: encryptFile failed, copying encrypted file!!";
                    QFile::copy( previewFileNameBeforeEncryption, previewFileNameAfterEncryption );
                } else {
                    qDebug () << "encrypted preview image to " << previewFileNameAfterEncryption;
                }

                QFile::remove( previewFileNameBeforeEncryption );

                WickrCore::FetchInformation previewfetchinfo;

                previewfetchinfo.guid = resourceName;
                previewfetchinfo.segment = 1;
                previewfetchinfo.key = previewKeyAES;
                QList <WickrCore::FetchInformation> previewfetchList;
                previewfetchList.append(previewfetchinfo);

                qint64 previewFileSize = 0;
                QFileInfo previewFileInfo(previewFileNameAfterEncryption);
                if(previewFileInfo.exists())
                {
                    previewFileSize = previewFileInfo.size();
                }

                WickrCore::Dimension previewDim;
                previewDim.height = result.height();
                previewDim.width = result.width();
                WickrCore::FileMetaData previewMeta("image/png", previewFileSize, previewfetchList, previewHash, previewDim);

                // create the thumbnail from the preview size
                result = result.scaledToHeight(maxThumbHeight, Qt::SmoothTransformation);
                if(result.width() > maxThumbWidth)
                {
                    result = orig.scaledToWidth(maxThumbWidth, Qt::SmoothTransformation);
                }

                resourceName = QUuid::createUuid().toString().mid(1,36).toUpper();

                QByteArray thumbnailKeyAES = convertCFDataToByteArray( ::randomLocalKey(), false );
                QString thumbnailFileNameAfterEncryption = WickrAppContext::getFilesDir() + resourceName;
                QString thumbnailFileNameBeforeEncryption = thumbnailFileNameAfterEncryption + "_tmp" ;
                //qDebug() << "guid:" << resourceName << "key:" << thumbnailKeyAES.toHex();

                //qDebug () << "saving thumbnail image as " << thumbnailFileNameBeforeEncryption;
                result.save(thumbnailFileNameBeforeEncryption, "PNG");

                // ENCRYPT THUMBNAIL FILE HERE
                // Disk encryption can overwrite the output of result.save
                //  -or-
                // Memory encryption can get the file contents of result, encrypt it, and save it
                QString thumbHash = cloudMgr->encryptFile( thumbnailKeyAES, thumbnailFileNameBeforeEncryption, thumbnailFileNameAfterEncryption );
                if (thumbHash.isEmpty()) {
                    qDebug() << "WickrIOActionHdlr::sendFile: encryptFile failed, copying encrypted file!!";
                    QFile::copy( thumbnailFileNameBeforeEncryption, thumbnailFileNameAfterEncryption );
                } else {
                    qDebug () << "encrypted thumbnail image to " << thumbnailFileNameAfterEncryption;
                }

                QFile::remove( thumbnailFileNameBeforeEncryption );

                qint64 thumbnailFileSize = 0;
                QFileInfo thumbnailFileInfo(thumbnailFileNameAfterEncryption);
                if(thumbnailFileInfo.exists())
                {
                    thumbnailFileSize = thumbnailFileInfo.size();
                }

                WickrCore::FetchInformation thumbfetchinfo;

                thumbfetchinfo.guid = resourceName;
                thumbfetchinfo.segment = 1;
                thumbfetchinfo.key = thumbnailKeyAES;
                QList <WickrCore::FetchInformation> thumbfetchList;
                thumbfetchList.append(thumbfetchinfo);

                WickrCore::Dimension thumbDim;
                thumbDim.height = result.height();
                thumbDim.width = result.width();

                WickrCore::FileMetaData thumbMeta("image/png", thumbnailFileSize, thumbfetchList, thumbHash, thumbDim);
//                    imageServer->addImage(resourceName, result);

                WickrCore::FileMetaData imageFileMetaData(metaDataMimeType, fileSize, fetchInfoList, imageHash, comments);

                // TODO: Need to upload specifi files
                WickrCore::FileInfo fileToUpload(name, imageFileMetaData);

                cloudMgr->uploadFile(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
            else
            {
                qDebug() << "can't seem to read image";
                WickrCore::FileInfo fileToUpload(name, fileMetaData);

                cloudMgr->uploadFile(targetConvo, fileNameAfterEncryption, fileToUpload);
            }
        }
        else
        {
            WickrCore::FileInfo fileToUpload(name, fileMetaData);

            cloudMgr->uploadFile(targetConvo, fileNameAfterEncryption, fileToUpload);
        }
        encryptionWatcher->deleteLater();
    });
    QFuture<QString> encryptionStep = QtConcurrent::run(&WickrCore::WickrCloudTransferMgr::encryptFile, encryptionKeyAES, fileNameBeforeEncryption, fileNameAfterEncryption);
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
//        m_operation->error(QString("Message send error: " + context->getResult()));
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

    // Update the Process status
    if ((m_timerStatsTicker % WICKRBOT_UPDATE_PROCESS_SECS) == 0) {
        m_operation->log("Updating process state");
        m_operation->updateProcessState(PROCSTATE_RUNNING);
    }

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

    if (m_operation->m_client != NULL) {
        gotAction = WickrBotActionDatabase::getFirstAction(m_operation->m_botDB, action, m_operation->m_client->id);
    } else {
        gotAction = WickrBotActionDatabase::getFirstAction(m_operation->m_botDB, action);
    }
    if (gotAction) {
        WickrBotJson *jsonHandler = new WickrBotJson();

        if (jsonHandler->parseJsonString(action->json)) {
            processAction(jsonHandler, action->id);
        }
    } else {
        m_processAction = false;
    }
    delete action;
}
