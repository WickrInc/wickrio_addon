#include <QJsonArray>
#include <QJsonDocument>

#include "wickrioreceivethread.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"
#include "user/wickrKeyVerificationMessage.h"

#include "common/wickrNotifyList.h"

#include "wickrIOMsgEmailService.h"
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
                m_operation->log("Messages received", msgs);
            }
            if (fails > 0) {
                m_operation->log("Messages received failed", fails);
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
 * @param msg
 * @return
 */
bool WickrIOReceiverMgr::dispatch(WickrCore::WickrInbox *msg)
{
    bool stillProcessing = true;
    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_messagesDropped++;
        return false;
    }

    m_messagesRecv++;

    QJsonObject jsonObject;

    WickrMsgClass mclass = msg->getMsgClass();
    jsonObject.insert(APIJSON_MSGTYPE, msg->getMessageType());

    if (mclass == MsgClass_Text) {
        QString txt = msg->getCachedText();
        jsonObject.insert(APIJSON_MESSAGE, txt);
    } else if (mclass == MsgClass_File) {
        if (!processFileMsg(jsonObject,  msg)) {
            //TODO: what to do if this returns false
            stillProcessing = false;
        }
    } else if (mclass == MsgClass_KeyVerification) {
        if (!processKeyVerificationMsg(jsonObject,  msg)) {
            //TODO: what to do if this returns false
            stillProcessing = false;
        }
    } else if (mclass == MsgClass_Control) {
        if (!processControlMsg(jsonObject,  msg)) {
            //TODO: what to do if this returns false
            stillProcessing = false;
        }
    }

    if (stillProcessing) {

        jsonObject.insert(APIJSON_VGROUPID, msg->getvGroupID());

        // Get the sender of this message
        WickrCore::WickrUser *sender = msg->getSenderUser();
        jsonObject.insert(APIJSON_MSGSENDER, sender->getUserID());

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

        QString respondApiText = m_operation->getResponseURL();
        if (!respondApiText.isEmpty()) {
            jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
        }

        QJsonDocument saveDoc(jsonObject);

        int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(), (int)msg->getMsgClass(), 0);
        WickrIOClientRuntime::cbSvcMessagesPending();
    }
    return stillProcessing;
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
    WickrCore::WickrControlMessage *ctrlMsg = WickrCore::WickrControlMessage::constructControlMessage(msg->getMsgBody());
    if (!ctrlMsg)
        return false;

    switch (ctrlMsg->getMsgType()) {
    case WickrCore::WickrControlMessage::CREATEROOM:
        return processCreateSecureRoomMsg(jsonObject, (WickrCore::WickrGroupControlCreateSecureRoom *)ctrlMsg);
    case WickrCore::WickrControlMessage::CHANGEMEMBERS:
        break;
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
#if 0
        QJsonObject member;

        member.insert(APIJSON_CTRL_MBRUID, mbr->getUid());
        member.insert(APIJSON_CTRL_MBRUNAME, mbr->getUname());
        member.insert(APIJSON_CTRL_MBRUKEY, QString(mbr->getUkey().toHex()));
        members.append(member);
#else
        members.append(mbr->getUname());
#endif
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
        addedUsers.append(QJsonValue(entry));
    }
    ctrlJsonObject.insert(APIJSON_CTRL_ADDUSERS, addedUsers);

    QJsonArray deletedUsers;
    for (QString entry: ctrlMsg->getDeletedUsers()) {
        deletedUsers.append(QJsonValue(entry));
    }
    ctrlJsonObject.insert(APIJSON_CTRL_DELUSERS, deletedUsers);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}


bool
WickrIOReceiverMgr::processFileMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
{
    return true;
#if 0
            WickrIORxDownloadFile *rxDownload = NULL;

            // Check if we are already downloading or decrypting this file
            if (m_activeDownloads.contains(msg->getMsgIDSecure())) {
                rxDownload = m_activeDownloads.value(msg->getMsgIDSecure(), NULL);
            } else if (m_activeDownloads.size() == 0){
                if (!msg->getFileInfo().isEmpty()) {
                    WickrCore::FileInfo fileInfo = msg->getFileInfo().at(0);

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
                    rxDownload = new WickrIORxDownloadFile(msg, fileInfo, attachmentFileName, realFileName);
                    m_activeDownloads.insert(msg->getMsgIDSecure(), rxDownload);
                } else {
                    qDebug() << "FileInfo is not set!";
                    deleteMsg = true;
                }
            }

            if (rxDownload != NULL) {
                WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
                if (cloudMgr) {
                    if (! rxDownload->m_downloaded) {
                        if (rxDownload->m_downloading) {
                            // Check if the download has completed
                            QFileInfo f(rxDownload->m_attachmentFileName);
                            if (f.exists()) {
                                rxDownload->m_downloaded = true;
                                rxDownload->m_downloading = false;
                            }
                        } else {
                            rxDownload->m_downloading = true;
                            cloudMgr->downloadFile(msg->getConvo(), rxDownload->m_attachmentFileName, rxDownload->m_fileInfo);
                        }
                    }
                } else {

                }

                // If done downloading and decrypting then pass off
                if (rxDownload != NULL && rxDownload->m_downloaded) {
                    int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(), (int)msg->getMsgClass(), 1);
                    db->insertAttachment(msgID, rxDownload->m_attachmentFileName, rxDownload->m_realFileName);
                    m_activeDownloads.remove(msg->getMsgIDSecure());
                    deleteMsg = true;
                }
            }
#endif

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
