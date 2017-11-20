#include <QJsonArray>
#include <QJsonDocument>

#include "wickrioreceivethread.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"
#include "user/wickrKeyVerificationMessage.h"
#include "calling/wickraudiovideocontrolmessage.h"

#include "common/wickrNotifyList.h"

#include "wickrIOClientRuntime.h"

#include "common/wickrRuntime.h"

#define WICKRBOT_UPDATE_STATS_SECS      600

WickrIOReceiveThread::WickrIOReceiveThread() :
    WickrIOThread(),
    m_enableSwitchboard(false),
    m_receiving(false),
    m_timerStatsTicker(0)
{
    m_operation = WickrIOClientRuntime::operationData();
}

WickrIOReceiveThread::~WickrIOReceiveThread()
{
    slotStopReceiving();
}

/**
 * @brief WickrIOReceiveThread::onTimerAction
 * This function is called when a timer action goes off.  This should happen
 * one time per second.  Currently this will log statistics.
 */
void WickrIOReceiveThread::onTimerAction()
{
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRBOT_UPDATE_STATS_SECS) == 0) {
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            int msgs = m_msgReceiver.messagesReceived();
            int fails = m_msgReceiver.messagesFailed();
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_MSGS_RX, DB_STATDESC_TOTAL, msgs);

            if (msgs > 0) {
                m_operation->log_handler->log("Messages received", msgs);
            }
            if (fails > 0) {
                m_operation->log_handler->log("Messages received failed", fails);
            }
        }
    }
}

/**
 * @brief WickrIOReceiveThread::processStarted
 * This function is called when the receive thread is started.  Initialize things
 * and be prepared to start receiving messages.
 */
void WickrIOReceiveThread::processStarted()
{
    qDebug() << "Started WickrIOReceiveThread";

    // Login successful, so login to switchboard
    startSwitchboard();

    emit signalProcessStarted();
}

/**
 * @brief WickrIOReceiveThread::slotStartReceiving
 * The main thread will call this to start receiving messages
 */
void
WickrIOReceiveThread::slotStartReceiving()
{
    // Make sure we are not already receiving
    if (! m_receiving) {
        m_receiving = true;

        // hook in to receive messages, let the show begin!
        WickrCore::WickrRuntime::registerMessageManager(&m_msgReceiver);
    }
    emit signalReceivingStarted();
}

/**
 * @brief WickrIOReceiveThread::slotStopReceiving
 * The main thread will call this to start receiving messages
 */
void
WickrIOReceiveThread::slotStopReceiving()
{
    if (m_receiving) {
        m_receiving = false;

        // hook in to receive messages, let the show begin!
        WickrCore::WickrRuntime::registerMessageManager(nullptr);
    }
    emit signalReceivingEnded();
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOReceiverMgr::WickrIOReceiverMgr
 * Constructure for the WickrIO compliance bot message receiver
 */
WickrIOReceiverMgr::WickrIOReceiverMgr() :
    m_messagesRecv(0),
    m_messagesDropped(0),
    m_messagesRecvFailed(0)
{
    m_operation = WickrIOClientRuntime::operationData();
}

/**
 * @brief WickrIOReceiverMgr::dispatch
 * This is the callback to receive messages
 * Returning true will mean this process is responsible to delete the message
 * @param msg
 * @return true if still using the msg, false if done with it
 */
bool WickrIOReceiverMgr::dispatch(WickrCore::WickrMessage *msg)
{
    bool extendProcessing = false;
    bool failedProcessing = false;

    // Do not handle outbox sync messages
    if (msg->isSyncedOutboxConversion()) {
        return false;
    }

    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_messagesDropped++;
//        msg->doDelete();
//        msg->release();
        return false;
    }

    m_messagesRecv++;

    QJsonObject jsonObject;

    /*
     * Insert common things into the JSON, that is found in all messages
     */
    jsonObject.insert(APIJSON_VGROUPID, msg->getvGroupID());

    // Get the sender of this message
    WickrCore::WickrUser *sender = msg->getSenderUser();
    jsonObject.insert(APIJSON_MSGSENDER, sender->getUserID());

    if (msg->getMsgBody().targetusers_size()) {
        const QString receiver = QString::fromStdString(msg->getMsgBody().targetusers(0));
        if (!receiver.isEmpty())
            jsonObject.insert(APIJSON_MSGRECEIVER, receiver);
    }

    // Add the message ID
    jsonObject.insert(APIJSON_MSGID, msg->getSrvMsgID());

#if 0
    WickrCore::WickrConvo* pConvo;
    if (pConvo) {
        // Setup the users array
        QList<WickrCore::WickrUser *>users = pConvo->getAllUsers();
        QJsonArray usersArray;

        for (WickrCore::WickrUser *user : users) {
            QJsonObject userEntry;

            userEntry.insert(APIJSON_NAME, user->getUserID());
            usersArray.append(userEntry);
        }

        jsonObject.insert(APIJSON_USERS, usersArray);
    }
#endif

    // Message timestamp
    long timestamp = msg->getMsgTimestamp();
    QDateTime msgDate;
    msgDate.setMSecsSinceEpoch((quint64)timestamp * 1000l);
    // get the "hh:mm ap" format based on locale
    QString statusText = msgDate.toString( QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    jsonObject.insert(APIJSON_MSGTIME, statusText);

    // Message micro seconds
    long usec = msg->getMsgUsec();
    QString msg_ts = QString("%1.%2").arg(timestamp).arg(usec);
    jsonObject.insert(APIJSON_MSG_TS, msg_ts);


#if 0
    // NOT SUPPORTED RIGHT NOW
    QString respondApiText = m_operation->getResponseURL();
    if (!respondApiText.isEmpty()) {
        jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
    }
#endif

    /*
     * Insert message specific stuff
     */
    WickrCore::WickrInbox *inbox = static_cast<WickrCore::WickrInbox*>(msg);

    WickrMsgClass mclass = msg->getMsgClass();
    jsonObject.insert(APIJSON_MSGTYPE, msg->getMessageType());

    if (mclass == MsgClass_Text) {
        QString txt = msg->getCachedText();
        jsonObject.insert(APIJSON_MESSAGE, txt);
    } else if (mclass == MsgClass_File) {
        if (!processFileMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        } else {
            // For file messages the content is sent to another thread
            extendProcessing = true;
        }
    } else if (mclass == MsgClass_KeyVerification) {
        if (!processKeyVerificationMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        }
    } else if (mclass == MsgClass_Control) {
        if (!processControlMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        }
    } else if (mclass == MsgClass_Call) {
        if (!processCallingMsg(jsonObject, inbox)) {
            failedProcessing = true;
        }
    }


    if (! extendProcessing && ! failedProcessing) {
        QJsonDocument saveDoc(jsonObject);

        int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(QJsonDocument::Compact), (int)msg->getMsgClass(), 0);
        WickrIOClientRuntime::cbSvcMessagesPending();
    }

    return extendProcessing;
}

bool
WickrIOReceiverMgr::processKeyVerificationMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
{
    WickrCore::WickrKeyVerificationMessage *kvMsg = WickrCore::WickrKeyVerificationMessage::constructKeyVerificationMessage(msg->getMsgBody());
    QJsonObject verifyJsonObject;

    verifyJsonObject.insert(APIJSON_KEYVER_MSGTYPE, kvMsg->getMsgType());

    if (!kvMsg->getKey().isEmpty()) {
        verifyJsonObject.insert(APIJSON_KEYVER_KEY, QString(kvMsg->getKey().toHex()));
    }
    if (!kvMsg->getHash().isEmpty()) {
        verifyJsonObject.insert(APIJSON_KEYVER_HASH, kvMsg->getHash());
    }
    if (!kvMsg->getReason().isEmpty()) {
        verifyJsonObject.insert(APIJSON_KEYVER_REASON, kvMsg->getReason());
    }
    if (!kvMsg->getVerifiedKey().isEmpty()) {
        verifyJsonObject.insert(APIJSON_KEYVER_VERKEY, QString(kvMsg->getVerifiedKey().toHex()));
    }

    jsonObject.insert(APIJSON_KEYVER_HEADER, verifyJsonObject);
    return true;
}

bool
WickrIOReceiverMgr::processControlMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
{
    WickrCore::WickrControlMessage *ctrlMsg = WickrCore::WickrControlMessage::constructControlMessage(msg->getMsgBody(), msg->getvGroupID());
    if (!ctrlMsg)
        return false;

    switch (ctrlMsg->getMsgType()) {
    case WickrCore::WickrControlMessage::CREATEROOM:
        return processCreateSecureRoomMsg(jsonObject, (WickrCore::WickrGroupControlCreateSecureRoom *)ctrlMsg);
    case WickrCore::WickrControlMessage::CHANGEMEMBERS:
        return processChangeMembersMsg(jsonObject,  (WickrCore::WickrGroupControlChangeMembers *)ctrlMsg);
    case WickrCore::WickrControlMessage::LEAVE:
        break;
    case WickrCore::WickrControlMessage::CHANGEPARMS:
        return processChangeRoomConfigMsg(jsonObject, (WickrCore::WickrGroupControlChangeRoomConfiguration *)ctrlMsg);
    case WickrCore::WickrControlMessage::DELETEROOM:
        break;
    case WickrCore::WickrControlMessage::SYNC_RECOVERY_REQ:
        break;
    case WickrCore::WickrControlMessage::SYNC_RECOVERY_RSP:
        break;
    case WickrCore::WickrControlMessage::SYNC_HISTORY_REQ:
        break;
    case WickrCore::WickrControlMessage::SYNC_HISTORY_RSP:
        break;
    default:
        return false;
    }

    return true;
}

bool
WickrIOReceiverMgr::processCreateRoomBase(QJsonObject& jsonObject,  WickrCore::WickrGroupControlCreateSecureRoom *ctrlMsg)
{
    QJsonArray members;
    for (WickrCore::WickrMemberInfo *mbr : ctrlMsg->getMembers()) {
        members.append(mbr->getUname());
    }
    jsonObject.insert(APIJSON_CTRL_MEMBERS, members);

    QJsonArray masters;
    for (QString mstr: ctrlMsg->getMastersList()) {
        // Find the member entry in the memberlist
        for (WickrCore::WickrMemberInfo *mbr : ctrlMsg->getMembers()) {
            if (mbr->getUid() == mstr) {
                masters.append(mbr->getUname());
                break;
            }
        }
    }
    jsonObject.insert(APIJSON_CTRL_MASTERS, masters);

    jsonObject.insert(APIJSON_CTRL_TTL, ctrlMsg->getDestructionTime());
    jsonObject.insert(APIJSON_CTRL_BOR, ctrlMsg->getBORTime());
    jsonObject.insert(APIJSON_CTRL_TITLE, ctrlMsg->getRoomTitle());
    jsonObject.insert(APIJSON_CTRL_DESC, ctrlMsg->getRoomDescription());
    jsonObject.insert(APIJSON_CTRL_CHGMASK, (int)ctrlMsg->getChangeMask());

    return true;
}

bool
WickrIOReceiverMgr::processCreateSecureRoomMsg(QJsonObject& jsonObject, WickrCore::WickrGroupControlCreateSecureRoom *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_CreateRoom);

    processCreateRoomBase(ctrlJsonObject, ctrlMsg);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}

bool
WickrIOReceiverMgr::processChangeRoomConfigMsg(QJsonObject& jsonObject, WickrCore::WickrGroupControlChangeRoomConfiguration *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_ModifyRoomParams);

    processCreateRoomBase(ctrlJsonObject, ctrlMsg);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}

bool
WickrIOReceiverMgr::processChangeMembersMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlChangeMembers *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_ModifyRoomMembers);

    QJsonArray addedUsers;
    for (QString entry: ctrlMsg->getAddedUsers()) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserByServerID(entry);
        if (user == nullptr || user->getUserID().isEmpty()) {
            addedUsers.append(QJsonValue(entry));
        } else {
            addedUsers.append(user->getUserID());
        }
    }
    ctrlJsonObject.insert(APIJSON_CTRL_ADDUSERS, addedUsers);

    QJsonArray deletedUsers;
    for (QString entry: ctrlMsg->getDeletedUsers()) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserByServerID(entry);
        if (user == nullptr || user->getUserID().isEmpty()) {
            deletedUsers.append(QJsonValue(entry));
        } else {
            deletedUsers.append(user->getUserID());
        }
    }
    ctrlJsonObject.insert(APIJSON_CTRL_DELUSERS, deletedUsers);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}


bool
WickrIOReceiverMgr::processFileMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
{
    WickrIORxDownloadFile *rxDownload = NULL;

    if (msg->getFileInfo().count() == 0) {
        return false;
    }

    WickrIOFileDownloadService *fileDescSvc = WickrIOClientRuntime::fdSvc();
    if (fileDescSvc == nullptr)
        return false;

    WickrCore::FileInfo fileInfo = msg->getFileInfo().at(0);
    QString file_guid = fileInfo.metaData().fetchInfo().at(0).guid;

    if (file_guid.isEmpty()) {
        return false;
    }

    QString dLoadFileName = fileInfo.fileName();
    QString realFileName;
    QString extension;
    if (!dLoadFileName.isEmpty()) {
        extension = "_" + dLoadFileName;
        realFileName = dLoadFileName;
    } else {
        if(fileInfo.metaData().mimeType() == "image/png")
            extension = ".png";
        else if (fileInfo.metaData().mimeType() == "image/jpeg")
            extension = ".jpeg";
        else if (fileInfo.metaData().mimeType() == "image/bmp")
            extension = ".bmp";
        else if (fileInfo.metaData().mimeType() == "image/gif")
            extension = ".gif";
    }

    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyyMMddhhmmsszzz");

    QString dirName = m_operation->attachmentsDir;

    if (dirName.isEmpty()) {
        // Save the attachment to the temp dir
        dirName = QDir::tempPath();
    }

    QString attachmentFileName(dirName +
#ifdef Q_OS_LINUX
            "/" +
#endif
     "attachment_" + dateTimeString + extension);

    if (realFileName.isEmpty())
        realFileName = attachmentFileName;


    // Insert the file message into the json object
    QJsonObject fileJsonObject;
    fileJsonObject.insert(APIJSON_FILE_LOCALFILE, attachmentFileName);
    fileJsonObject.insert(APIJSON_FILE_FILENAME, dLoadFileName);
    fileJsonObject.insert(APIJSON_FILE_GUID, file_guid);
    jsonObject.insert(APIJSON_FILE_HEADER, fileJsonObject);

    rxDownload = new WickrIORxDownloadFile(msg, fileInfo, attachmentFileName, realFileName, jsonObject);

    if (rxDownload != NULL) {
        fileDescSvc->downloadFile(rxDownload);
    } else {
        return false;
    }

    return true;
}

QString
WickrIOReceiveThread::getAttachmentFile(const QByteArray &data, QString extension)
{
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyyMMddhhmmsszzz");

    QString dirName = m_operation->attachmentsDir;

    if (dirName.isEmpty()) {
        // Save the attachment to the temp dir
        dirName = QDir::tempPath();
    }

    QString attachmentFileName(dirName +
#ifdef Q_OS_LINUX
            "/" +
#endif
     "attachment_" + dateTimeString + "." + extension);

    QFile tempFile(attachmentFileName);
    tempFile.open(QIODevice::WriteOnly);
    tempFile.write(data);
    tempFile.close();
    return attachmentFileName;
}

bool
WickrIOReceiverMgr::processCallingMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
{
    WickrCore::WickrAudioVideoControlMessage *avMsg = WickrCore::WickrAudioVideoControlMessage::create( msg->getMsgBody() );
    QJsonObject verifyJsonObject;

    if (avMsg) {
        verifyJsonObject.insert(APIJSON_CALL_STATUS, avMsg->status());

        if (!avMsg->getCallId().isEmpty()) {
            verifyJsonObject.insert(APIJSON_CALL_MEETINGID, avMsg->getCallId());
        }
    }

    jsonObject.insert(APIJSON_CALL_HEADER, verifyJsonObject);
    return true;
}




void WickrIOReceiveThread::startSwitchboard()
{
    // Update switchboard login credentials (login is performed only if not already logged in)
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin(),
                                         true);
}

void WickrIOReceiveThread::stopSwitchboard()
{
    WickrCore::WickrRuntime::swbSvcLogout();
}
