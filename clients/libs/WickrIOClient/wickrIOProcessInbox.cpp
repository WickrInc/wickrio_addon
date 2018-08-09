#include <QJsonArray>

#include "wickrIOProcessInbox.h"
#include "wickrioapi.h"
#include "wickrIOFileDownloadService.h"
#include "wickrIOClientRuntime.h"

#include "user/wickrKeyVerificationMgr.h"
#include "user/wickrKeyVerificationMessage.h"
#include "calling/wickraudiovideocontrolmessage.h"

bool
WickrIOProcessInbox::processCommonFields(QJsonObject& jsonObject, WickrCore::WickrMessage *msg)
{
    jsonObject.insert(APIJSON_VGROUPID, msg->getvGroupID());

    // Add the message ID
    jsonObject.insert(APIJSON_MSGID, msg->getSrvMsgID());

    // Add the message type
    jsonObject.insert(APIJSON_MSGTYPE, msg->getMessageType());

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


    // Get the sender of this message
    WickrCore::WickrUser *sender = msg->getSenderUser();
    jsonObject.insert(APIJSON_MSGSENDER, sender->getUserID());

    return true;
}

bool
WickrIOProcessInbox::processKeyVerificationMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
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
WickrIOProcessInbox::processControlMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
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
    case WickrCore::WickrControlMessage::DELETEMSG:
        return processDeleteMessageMsg(jsonObject, (WickrCore::WickrGroupControlDeleteMessage *)ctrlMsg);
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
WickrIOProcessInbox::processDeleteMessageMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlDeleteMessage *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_DeleteMessage);
    ctrlJsonObject.insert(APIJSON_CTRL_MSGID, ctrlMsg->msgID());
    ctrlJsonObject.insert(APIJSON_CTRL_ISRECALL, ctrlMsg->isRecall());

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}

bool
WickrIOProcessInbox::processCreateRoomBase(QJsonObject& jsonObject,  WickrCore::WickrGroupControlCreateSecureRoom *ctrlMsg)
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
WickrIOProcessInbox::processCreateSecureRoomMsg(QJsonObject& jsonObject, WickrCore::WickrGroupControlCreateSecureRoom *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_CreateRoom);

    processCreateRoomBase(ctrlJsonObject, ctrlMsg);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}

bool
WickrIOProcessInbox::processChangeRoomConfigMsg(QJsonObject& jsonObject, WickrCore::WickrGroupControlChangeRoomConfiguration *ctrlMsg)
{
    QJsonObject ctrlJsonObject;
    ctrlJsonObject.insert(APIJSON_CTRL_MSGTYPE, MsgType_Ctrl_ModifyRoomParams);

    processCreateRoomBase(ctrlJsonObject, ctrlMsg);

    jsonObject.insert(APIJSON_CTRL_HEADER, ctrlJsonObject);
    return true;
}

bool
WickrIOProcessInbox::processChangeMembersMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlChangeMembers *ctrlMsg)
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
WickrIOProcessInbox::processFileMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
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

    OperationData *operationPtr = WickrIOClientRuntime::operationData();
    QString dirName = operationPtr->attachmentsDir;

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
    if (fileInfo.metaData().isScreenshot()) {
        fileJsonObject.insert(APIJSON_FILE_ISSCRSHOT, true);
    }
    jsonObject.insert(APIJSON_FILE_HEADER, fileJsonObject);

    rxDownload = new WickrIORxDownloadFile(msg, fileInfo, attachmentFileName, realFileName, jsonObject);

    if (rxDownload != NULL) {
        fileDescSvc->downloadFile(rxDownload);
    } else {
        return false;
    }

    return true;
}

bool
WickrIOProcessInbox::processCallingMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg)
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

