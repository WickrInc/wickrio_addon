#ifndef WICKRIOPROCESSINBOX_H
#define WICKRIOPROCESSINBOX_H

#include "services/wickrMessageService.h"
#include "services/wickrSwitchboardService.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"

#include "common/wickrMessageMgr.h"
#include "messaging/wickrGroupControl.h"
#include "wickrIOFileDownloadService.h"


class WickrIOProcessInbox
{
public:
    WickrIOProcessInbox() {}

    static bool processCommonFields(QJsonObject& jsonObject, WickrCore::WickrMessage *msg);

    // Process inbound messages
    static bool processKeyVerificationMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg);
    static bool processFileMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg);
    static bool processCallingMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg);

    static bool processControlMsg(QJsonObject& jsonObject,  WickrCore::WickrInbox *msg);
    static bool processCreateRoomBase(QJsonObject& jsonObject,  WickrCore::WickrGroupControlCreateSecureRoom *ctrlMsg);
    static bool processCreateSecureRoomMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlCreateSecureRoom *creaatSecureRoom);
    static bool processChangeRoomConfigMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlChangeRoomConfiguration *ctrlMsg);
    static bool processChangeMembersMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlChangeMembers *ctrlMsg);

    static bool processDeleteMessageMsg(QJsonObject& jsonObject,  WickrCore::WickrGroupControlDeleteMessage *ctrlMsg);
};

#endif // WICKRIOPROCESSINBOX_H
