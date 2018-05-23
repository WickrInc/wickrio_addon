#include <QJsonArray>

#include "wickrIOAPIInterface.h"
#include "wickrIOErrorHandler.h"
#include "wickrbotjsondata.h"
#include "wickrioapi.h"
#include "wickriodatabase.h"

#include "common/wickrRuntime.h"


WickrIOAPIInterface::WickrIOAPIInterface(OperationData *operation, QObject* parent) :
    QObject(parent),
    m_operation(operation)
{
}

/**
 * @brief RequestHandler::getJsonArrayValue
 * This function will return a list of strings for the specified entry within
 * the named json array.
 * TODO: Move this to a JSON class
 * @param jsonObject the JSON object to parse from
 * @param jsonName the name of the values to pull
 * @param jsonTag the name of the array to use
 * @return
 */
QStringList
WickrIOAPIInterface::getJsonArrayValue(QJsonObject jsonObject, QString jsonName, QString jsonArray)
{
    QStringList retList;
    QJsonArray entryArray;
    QJsonValue value;

    // Parse the contacts
    if (jsonObject.contains(jsonArray)) {
        value = jsonObject[jsonArray];
        entryArray = value.toArray();

        for (int i=0; i< entryArray.size(); i++) {
            QJsonValue arrayValue;

            arrayValue = entryArray[i];

            if (arrayValue.isObject()) {
                // Get the title for this contact entry
                QJsonObject arrayObject = arrayValue.toObject();

                if (arrayObject.contains(jsonName)) {
                    QJsonValue idobj = arrayObject[jsonName];
                    QString id = idobj.toString();
                    retList.append(id);
                }
            }
        }
    }
    return retList;
}


bool
WickrIOAPIInterface::updateAndValidateMembers(QString& responseString, const QStringList& memberslist, QStringList *memberHashes)
{
    QStringList memberSearchList;
    foreach (QString member, memberslist) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(member);
        if (user == NULL) {
            memberSearchList.append(member);
        } else {
            int networkflag = user->getUserNetworkFlag();
            qDebug() << "Networkflag for " << member << " is " << networkflag;

            QString idHash = user->getServerIDHash();
            if (idHash.isEmpty()) {
                responseString = "Problem handling user record!";
                return false;
            }
            if (memberHashes != nullptr)
                memberHashes->append(idHash);
        }
    }

    // TODO: Need to check if is a master in the room
    if (memberSearchList.size() > 0) {

        // Post a request to th server for each member to search for
        foreach (QString searchMember, memberSearchList) {
            WickrUserValidateSearch *c = new WickrUserValidateSearch(WICKR_USERNAME_ALIAS,searchMember,0);
            QObject::connect(c, &WickrUserValidateSearch::signalRequestCompleted, [=](WickrUserValidateSearch *context) {
                emit signalMemberSearchDone();
                context->deleteLater();
            });
            WickrCore::WickrRuntime::taskSvcMakeRequest(c);

            QTimer timer;
            QEventLoop loop;

            loop.connect(this, SIGNAL(signalMemberSearchDone()), SLOT(quit()));
            connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

            int loopCount = 10;

            while (loopCount-- > 0) {
                timer.start(1000);
                loop.exec();

                if (timer.isActive()) {
                    timer.stop();
                    break;
                } else {
                    qDebug() << "CONSOLE:Timed out waiting for User Validate Search response!";
                    responseString = "Failure waiting for user verification";
                    return false;
                }
            }
        }

        // Now make sure that user records have been created for the searched members
        foreach (QString searchMember, memberSearchList) {
            WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(searchMember);
            if (user == NULL) {
                responseString = "Cannot find user record for " + searchMember;
                return false;
            }

            int networkflag = user->getUserNetworkFlag();
            qDebug() << "Networkflag for " << searchMember << " is " << networkflag;

            if (memberHashes != nullptr) {
                QString idHash = user->getServerIDHash();
                if (idHash.isEmpty()) {
                    responseString = "Problem handling user record for " + searchMember;
                    return false;
                }
                memberHashes->append(idHash);
            }
        }
    }
    return true;
}

/**
 * @brief WickrIOAPIInterface::deleteConvo
 * Will delete local convo (secure room or 1-to-1). If secure room, will kick off
 * deleteSecureRoomStart service to delete remotes.
 *
 */
bool
WickrIOAPIInterface::deleteConvo(bool isSecureConvo, const QString& vgroupID)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vgroupID);
    if (convo) {
        if (isSecureConvo) {
            if (convo->getIsRoomMaster()) {
                WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
                if (roomMgr) {
                    roomMgr->deleteSecureRoomStart(convo);
                } else {
                    return false;
                }
            } else {
                m_operation->log_handler->error("DELETE SECURE ROOM: Failed\n You are not a room master of this secure room.");
                return false;
            }
        } else {
            WickrCore::WickrOneToOneMgr *one2oneMgr = WickrCore::WickrRuntime::getConvoMgr();
            if (one2oneMgr) {
                one2oneMgr->deleteOneToOneStart(vgroupID);
            } else {
                return false;
            }
        }
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

bool
WickrIOAPIInterface::sendMessage(const QByteArray& json, QString& responseString)
{
    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);
    bool retValue=false;

    if (! jsonHandler->parseJson4SendMessage(json)) {
        if (!jsonHandler->getLastError().isEmpty())
            responseString = jsonHandler->getLastError();
        else
            responseString = "Failed parsing message JSON";
    } else {
        // Check if there is a status user and if it is a valid user
        if (!jsonHandler->m_statususer.isEmpty()) {
            QStringList statusUList;
            statusUList.append(jsonHandler->m_statususer);
            if (!updateAndValidateMembers(responseString, statusUList)) {
                delete jsonHandler;
                return false;
            }
        }

        QStringList userNames = jsonHandler->getUserNames();
        QString vGroupID = jsonHandler->getVGroupID();

        // If there is a message and the length is too long return error
        if (!jsonHandler->m_message.isEmpty() &&
                (jsonHandler->m_message.length() > WickrCore::WickrRuntime::getEnvironmentMgr()->maxMessageSize())) {
            responseString = "Message length too long";
        }
        else if (userNames.isEmpty() && vGroupID.isEmpty()) {
            responseString = "No users identified";
        } else if (!userNames.isEmpty()){
            // Update and validate the input list of usernames.  If false is returned then
            // there was an error processing the list, or the user was invalid!
            if (updateAndValidateMembers(responseString, userNames)) {
                if (jsonHandler->postEntry4SendMessage()) {
                    retValue = true;
                } else {
                    responseString = "Failed sending message";
                }
            }
        } else {
            if (jsonHandler->postEntry4SendMessage()) {
                retValue = true;
            } else {
                responseString = "Failed sending message";
            }
        }
    }
    delete jsonHandler;

    m_operation->log_handler->log(retValue ? "Message parsed successfully" : responseString);
    return retValue;
}

bool
WickrIOAPIInterface::getReceivedMessages(QString& responseString)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        responseString = "Internal Error: Failed to cast database!";
        return false;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);

    // Make sure the start is less than the number of IDs retrieved
    if (msgIDs.size() == 0) {
        responseString = "{ \"}";
    } else {
        WickrIOMessage rxMsg;
        if (db->getMessage(msgIDs.at(0), &rxMsg)) {
            responseString = rxMsg.json;
            db->deleteMessage(rxMsg.id, true);
        }
     }
     return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


bool
WickrIOAPIInterface::addRoom(const QByteArray& json, QString& responseString)
{
    // Get the room details from the incoming message
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(json, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        responseString = "Failed to parse JSON";
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    QJsonObject roomObject;

    // Start the operation
    if (jsonObject.contains(APIJSON_ROOM)) {
        value = jsonObject[APIJSON_ROOM];
        roomObject = value.toObject();
    } else {
        responseString = "malformed JSON";
        return false;
    }

    // Check that required objects exist
    if (!roomObject.contains(APIJSON_ROOMMEMBERS) || !roomObject.contains(APIJSON_ROOMMASTERS) ||
        !roomObject.contains(APIJSON_ROOMTITLE))
    {
        responseString = "Missing required data";
        return false;
    }

    QStringList memberslist;
    QStringList memberHashes;
    QStringList masterslist;
    QString title;
    QString description;
    int ttl = 0;
    int bor = 0;

    // Get the TTL / Destruct time
    if (roomObject.contains(APIJSON_ROOMTTL)) {
        value = roomObject[APIJSON_ROOMTTL];
        ttl = value.toInt(0);
    }

    // Get the BOR / Burn on read
    if (roomObject.contains(APIJSON_ROOMBOR)) {
        value = roomObject[APIJSON_ROOMBOR];
        bor = value.toInt(0);
    }

    // Get the description
    if (roomObject.contains(APIJSON_ROOMDESC)) {
        value = roomObject[APIJSON_ROOMDESC];
        description = value.toString();
    }

    // Get the title
    value = roomObject[APIJSON_ROOMTITLE];
    title = value.toString();
    if (title.isEmpty()) {
        responseString = "Invalid title";
        return false;
    }

    // Get the members
    memberslist = getJsonArrayValue(roomObject, APIJSON_NAME, APIJSON_ROOMMEMBERS);
    QString members = memberslist.join(",");
    if (members.isEmpty()) {
        responseString = "Invalid members list";
        return false;
    }

    // Update and validate the input list of members.  If false is returned then
    // there was an error processing the list, or the member was invalid!
    if (!updateAndValidateMembers(responseString, memberslist, &memberHashes)) {
        return false;
    }

    // Get the list of masters
    masterslist = getJsonArrayValue(roomObject, APIJSON_NAME, APIJSON_ROOMMASTERS);
    if (masterslist.size() == 0) {
        responseString = "Invalid masters list";
        return false;
    }

    QStringList masterHashes;
    foreach (QString master, masterslist) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(master);
        if (user == NULL) {
            responseString = "Cannot find user record for master";
            return false;
        }
        QString idHash = user->getServerIDHash();
        if (idHash.isEmpty()) {
            responseString = "Problem handling user record for master";
            return false;
        }
        masterHashes.append(idHash);
    }

    // Create the room

    // Create convo (create if not found, no group name, secure room)
    QString mastersString = masterHashes.join(',');
    QString membersString = memberHashes.join(',');

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (!roomMgr) {
        responseString = "Failed to create room";
        return false;
    }

    QString vgroupID = roomMgr->createSecureRoomConvoStart(membersString,
                                                           mastersString,
                                                           ttl,
                                                           title,
                                                           description,
                                                           bor,
                                                           true);
    if (vgroupID.isEmpty()) {
        responseString = "Failed to create room";
        return false;
    }

    // no will wait for the room to be created or timeout due to an error

    QTimer timer;
    QEventLoop loop;

    loop.connect(roomMgr, SIGNAL(roomCreatedServerSuccess()), SLOT(quit()));
//        loop.connect(roomMgr, SIGNAL(signalError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 10;

    while (loopCount-- > 0) {
        timer.start(1000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "CONSOLE:Timed out waiting for create secure room response!";
            responseString = "Failed to create room";
            return false;
        }
    }

    // Return the vGroupID
    responseString = QString("{ \"vgroupid\" : \"%1\" }").arg(vgroupID);
    return true;
}

bool
WickrIOAPIInterface::updateRoom(const QString &vGroupID, const QByteArray& json, QString& responseString)
{
    // Get the room details from the incoming message
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(json, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        responseString = "JSON Parsing error";
        return false;
    }

    // Get the convo associated with the room to be changed
    WickrCore::WickrConvo* convo = WickrCore::WickrConvo::getConvoWithvGroupID( vGroupID );
    if (!convo) {
        responseString = "Convo does not exist";
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    bool has_memberslist;
    QStringList memberslist;
    QStringList memberHashes;
    QStringList addMembersList;
    QStringList addMembersHashes;
    QStringList delMembersList;
    QStringList delMembersHashes;
    bool has_masterslist;
    QStringList masterslist;
    QStringList masterHashes;
    bool has_title;
    QString title;
    bool has_description;
    QString description;
    bool has_ttl;
    long ttl = 0;
    bool has_bor;
    long bor = 0;

    // Get the TTL / Destruct time
    has_ttl = jsonObject.contains(APIJSON_ROOMTTL);
    if (has_ttl) {
        value = jsonObject[APIJSON_ROOMTTL];
        ttl = value.toInt(0);
    } else {
        ttl = convo->getDestruct();
    }

    // Get the BOR / Burn on read
    has_bor = jsonObject.contains(APIJSON_ROOMBOR);
    if (has_bor) {
        value = jsonObject[APIJSON_ROOMBOR];
        bor = value.toInt(0);
    } else {
        bor = convo->getBOR();
    }

    // Get the description
    has_description = jsonObject.contains(APIJSON_ROOMDESC);
    if (has_description) {
        value = jsonObject[APIJSON_ROOMDESC];
        description = value.toString();
    } else {
        description = convo->getRoomPurpose();
    }

    // Get the title
    has_title = jsonObject.contains(APIJSON_ROOMTITLE);
    if (has_title) {
        value = jsonObject[APIJSON_ROOMTITLE];
        title = value.toString();
        if (title.isEmpty()) {
            responseString = "Invalid title";
            return false;
        }
    } else {
        title = convo->getVGroupTag();
    }

    // Get the members
    has_memberslist = jsonObject.contains(APIJSON_ROOMMEMBERS);
    if (has_memberslist) {
        memberslist = getJsonArrayValue(jsonObject, APIJSON_NAME, APIJSON_ROOMMEMBERS);
        QString members = memberslist.join(",");
        if (members.isEmpty()) {
            responseString = "Invalid members list";
            return false;
        }

        // Update and validate the input list of members.  If false is returned then
        // there was an error processing the list, or the member was invalid!
        if (!updateAndValidateMembers(responseString, memberslist, &memberHashes)) {
            return false;
        }

        QStringList allUserNames;
        QList<WickrCore::WickrUser *>users = convo->getAllUsers();
        for (WickrCore::WickrUser *user : users) {
            allUserNames.append(user->getUserID());
        }
        addMembersList = memberslist;
        for (QString userName : allUserNames)
            addMembersList.removeOne(userName);
        delMembersList = allUserNames;
        for (QString userName : memberslist)
            delMembersList.removeOne(userName);

        // Update and validate the input list of members.  If false is returned then
        // there was an error processing the list, or the member was invalid!
        if (!updateAndValidateMembers(responseString, addMembersList, &addMembersHashes)) {
            return false;
        }
        // Update and validate the input list of members.  If false is returned then
        // there was an error processing the list, or the member was invalid!
        if (!updateAndValidateMembers(responseString, delMembersList, &delMembersHashes)) {
            return false;
        }
    }

    // Get the list of masters
    has_masterslist = jsonObject.contains(APIJSON_ROOMMASTERS);
    if (has_masterslist) {
        masterslist = getJsonArrayValue(jsonObject, APIJSON_NAME, APIJSON_ROOMMASTERS);
        if (masterslist.size() == 0) {
            responseString = "Invalid masters list";
            return false;
        }

        foreach (QString master, masterslist) {
            WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(master);
            if (user == NULL) {
                responseString = "Cannot find user record for master";
                return false;
            }
            QString idHash = user->getServerIDHash();
            if (idHash.isEmpty()) {
                responseString = "Problem handling user record for master";
                return false;
            }
            masterHashes.append(idHash);
        }
    }

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (!roomMgr) {
        responseString = "Failed to create room";
        return false;
    }

    // Update the room

    // Create convo (create if not found, no group name, secure room)
    QString mastersString = masterHashes.join(',');
    QString membersString = memberHashes.join(',');

    QString addUsersHash = addMembersHashes.join(',');
    QString delUsersHash = delMembersHashes.join(',');

    int roomFunction = 0;

    if (has_memberslist || has_masterslist)
        roomFunction |= WickrCore::WickrSecureRoomMgr::EDIT_MEMBERS;
    if (has_bor || has_ttl || has_title || has_description)
        roomFunction |= WickrCore::WickrSecureRoomMgr::EDIT_CONFIG;

    if (! roomMgr->editSecureRoomStart(roomFunction,
                                       vGroupID,
                                       addUsersHash,
                                       delUsersHash,
                                       ttl,
                                       title,
                                       description,
                                       masterHashes,
                                       bor,
                                       false)) {
        responseString = "Failed to modify room";
        return false;
    }

    // no will wait for the room to be created or timeout due to an error

    QTimer timer;
    QEventLoop loop;

    loop.connect(roomMgr, SIGNAL(roomCreatedServerSuccess()), SLOT(quit()));
//        loop.connect(roomMgr, SIGNAL(signalError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 10;

    while (loopCount-- > 0) {
        timer.start(1000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "CONSOLE:Timed out waiting for create secure room response!";
            responseString = "Failed to create room";
            return false;
        }
    }

    return true;
}

bool
WickrIOAPIInterface::deleteRoom(const QString &vGroupID, QString& responseString)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo) {
        if (convo->getConvoType() != CONVO_SECURE_ROOM) {
            responseString = "Must be secure room";
        } else if (!convo->getIsRoomMaster()) {
            responseString = "Must be room master";
        } else {
            if (WickrIOAPIInterface::deleteConvo(true, vGroupID)) {
                return true;
            } else {
                responseString = "Failed to delete room";
            }
        }
    }
    return false;
}

bool
WickrIOAPIInterface::leaveRoom(const QString &vGroupID, QString& responseString)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo) {
        if (convo->getConvoType() == CONVO_SECURE_ROOM || convo->getConvoType() == CONVO_GROUP_CONVO) {
            WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
            if (roomMgr) {
                roomMgr->leaveSecureRoomStart(vGroupID);
                return true;
            } else {
                // 500 error
                responseString = "Internal error";
            }
        } else {
            responseString = "Can only leave secure room or group convo";
        }
    } else {
        responseString = "Cannot find convo for vgroupid";
    }
    return false;
}

QJsonObject
WickrIOAPIInterface::getRoomInfo(WickrCore::WickrConvo *convo)
{
    QJsonObject roomValue;

    // If a secure room then add to the list
    if (WickrCore::WickrConvo::getConvoTypeFromVGroupID(convo->getVGroupID()) == CONVO_SECURE_ROOM &&
        convo->isSecureRoomStateSynced()) {

        roomValue.insert(APIJSON_ROOMTITLE, convo->getVGroupTag());
        roomValue.insert(APIJSON_ROOMDESC, convo->getRoomPurpose());
        roomValue.insert(APIJSON_VGROUPID, convo->getVGroupID());
        roomValue.insert(APIJSON_ROOMTTL, QString::number(convo->getDestruct()));
        roomValue.insert(APIJSON_ROOMBOR, QString::number(convo->getBOR()));

        // Setup the users array
        QStringList masterslist = convo->getRoomMastersUserName();
        QJsonArray mastersArray;

        for (QString master : masterslist) {
            QJsonObject masterEntry;

            masterEntry.insert(APIJSON_NAME, master);
            mastersArray.append(masterEntry);
        }
        roomValue.insert(APIJSON_ROOMMASTERS, mastersArray);

        // Setup the users array
        QStringList userslist = convo->getUsernameStringArray();
        QJsonArray usersArray;

        for (QString user : userslist) {
            QJsonObject userEntry;

            userEntry.insert(APIJSON_NAME, user);
            usersArray.append(userEntry);
        }
        roomValue.insert(APIJSON_ROOMMEMBERS, usersArray);
    }
    return roomValue;
}

bool
WickrIOAPIInterface::getRoom(const QString &vGroupID, QString& responseString)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo != nullptr) {
        QJsonObject roomValue = getRoomInfo(convo);
        if (roomValue.isEmpty()) {
            responseString = "Convo is not a secure room";
        } else {
            QJsonDocument saveDoc(roomValue);
            QByteArray byteArray = saveDoc.toJson();

            responseString = byteArray;
            return true;
        }
    } else {
        responseString = "Can not find convo";
    }
    return false;
}

bool
WickrIOAPIInterface::getRooms(QString& responseString)
{
#if 0
    QByteArray paramStart = request.getParameter(APIPARAM_START);
    QByteArray paramCount = request.getParameter(APIPARAM_COUNT);

    if (paramStart.length() == 0 || paramCount.length() == 0) {
        sendFailure(400, "Missing parameters", response);
        return;
    }
    int start = paramStart.toInt();
    int count = paramCount.toInt();
#endif
    QJsonArray roomArrayValue;

    WickrConvoList *curConvos = WickrCore::WickrConvo::getConvoList();

    // TODO: Should there be a semaphore to control access to this?
    if (curConvos != NULL) {
        QList<QString> keys = curConvos->acquireKeyList();

        // Process each of the convos
        for (int cid=0; cid < keys.size(); cid++) {

            WickrCore::WickrConvo *currentConvo = (WickrCore::WickrConvo *)curConvos->value( keys.at(cid) );
            QJsonObject roomValue = getRoomInfo(currentConvo);
            if (!roomValue.isEmpty())
                roomArrayValue.append(roomValue);
        }
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_ROOMS, roomArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();
    responseString = byteArray;
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

bool
WickrIOAPIInterface::addGroupConvo(const QByteArray& json, QString& responseString)
{
    // Get the room details from the incoming message
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(json, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        responseString = "Failed";
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    QJsonObject grpConvoObject;

    // Start the operation
    if (jsonObject.contains(APIJSON_GROUPCONVO)) {
        value = jsonObject[APIJSON_GROUPCONVO];
        grpConvoObject = value.toObject();
    } else {
        responseString = "malformed JSON";
        return false;
    }

    // Check that required objects exist
    if (!grpConvoObject.contains(APIJSON_ROOMMEMBERS))
    {
        responseString = "Missing required data";
        return false;
    }

    QStringList memberslist;
    QStringList memberHashes;
    int ttl = 0;
    int bor = 0;

    // Get the TTL / Destruct time
    if (grpConvoObject.contains(APIJSON_ROOMTTL)) {
        value = grpConvoObject[APIJSON_ROOMTTL];
        ttl = value.toInt(0);
    }
    // Get the BOR / Burn on Read
    if (grpConvoObject.contains(APIJSON_ROOMBOR)) {
        value = grpConvoObject[APIJSON_ROOMBOR];
        bor = value.toInt(0);
    }

    // Get the members
    memberslist = getJsonArrayValue(grpConvoObject, APIJSON_NAME, APIJSON_ROOMMEMBERS);
    QString members = memberslist.join(",");
    if (members.isEmpty()) {
        responseString = "Invalid members list";
        return false;
    }

    // Update and validate the input list of members.  If false is returned then
    // there was an error processing the list, or the member was invalid!
    if (!updateAndValidateMembers(responseString, memberslist, &memberHashes)) {
        return false;
    }
    // Create the room

    // Create convo (create if not found, no group name, secure room)
    QString membersString = memberHashes.join(',');

    QSet<WickrCore::WickrUser *> validateUsers;

    // Need to make sure this convo does not exist already
    QList<WickrCore::WickrUser*>users;

    for (int i=0; i < memberslist.size(); i++) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(memberslist.at(i));
//        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserByServerID(memberslist.at(i));
        if (user) {
            users.append(user);
            validateUsers.insert(user);
        } else {
            QString errMsg = "Cannot find user record for " + memberslist.at(i);
            responseString = errMsg.toLatin1();
            return false;
        }
    }
    // If group already exists, simply redirect
    QString vgroupID = WickrCore::WickrConvo::groupConvoExists(users);
    if (!vgroupID.isEmpty()) {
        responseString = "Group conversation already exists";
        return false;
    }

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (!roomMgr) {
        responseString = "Failed to create group conversation";
        return false;
    }

    WickrCore::WickrUser *self = WickrCore::WickrUser::getSelfUser();
    QString mastersString;
    if (self) {
        mastersString = membersString + "," + self->getServerIDHash();
    } else {
        responseString = "Failed to create group conversation";
        return false;
    }

    vgroupID = roomMgr->createSecureRoomConvoStart(membersString,
                                                   mastersString,
                                                   ttl,
                                                   "",
                                                   "",
                                                   bor,
                                                   false);
    if (vgroupID.isEmpty()) {
        responseString = "Failed to create group conversation";
        return false;
    }

    // no will wait for the room to be created or timeout due to an error

    QTimer timer;
    QEventLoop loop;

    loop.connect(roomMgr, SIGNAL(roomCreatedServerSuccess()), SLOT(quit()));
//        loop.connect(roomMgr, SIGNAL(signalError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 10;

    while (loopCount-- > 0) {
        timer.start(1000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "CONSOLE:Timed out waiting for create group conversation response!";
            responseString = "Failed to create group conversation";
            return false;
        }
    }

    // Return the vGroupID
    responseString = QString("{ \"vgroupid\" : \"%1\" }").arg(vgroupID);
    return true;
}

bool
WickrIOAPIInterface::deleteGroupConvo(const QString &vGroupID, QString& responseString)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo) {
        if (convo->getConvoType() != CONVO_GROUP_CONVO) {
            responseString = "Must be group conversation";
        } else if (WickrIOAPIInterface::deleteConvo(false, vGroupID)) {
            return true;
        } else {
            responseString = "Failed to delete Group Convo";
        }
    }
    return false;
}

QJsonObject
WickrIOAPIInterface::getGroupConvoInfo(WickrCore::WickrConvo *convo)
{
    QJsonObject grpConvoValue;

    // If a secure room then add to the list
    if (WickrCore::WickrConvo::getConvoTypeFromVGroupID(convo->getVGroupID()) == CONVO_GROUP_CONVO) {
        grpConvoValue.insert(APIJSON_VGROUPID, convo->getVGroupID());
        grpConvoValue.insert(APIJSON_ROOMTTL, QString::number(convo->getDestruct()));
        grpConvoValue.insert(APIJSON_ROOMBOR, QString::number(convo->getBOR()));

        // Setup the users array
        QStringList userslist = convo->getUsernameStringArray();
        QJsonArray usersArray;

        for (QString user : userslist) {
            QJsonObject userEntry;

            userEntry.insert(APIJSON_NAME, user);
            usersArray.append(userEntry);
        }
        grpConvoValue.insert(APIJSON_ROOMMEMBERS, usersArray);
    }
    return grpConvoValue;
}

bool
WickrIOAPIInterface::getGroupConvo(const QString &vGroupID, QString& responseString)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo != nullptr) {
        QJsonObject roomValue = getGroupConvoInfo(convo);
        if (roomValue.isEmpty()) {
            responseString = "Convo is not a secure room";
        } else {
            QJsonDocument saveDoc(roomValue);
            QByteArray byteArray = saveDoc.toJson();

            responseString = byteArray;
            return true;
        }
    } else {
        responseString = "Can not find convo";
    }
    return false;
}

bool
WickrIOAPIInterface::getGroupConvos(QString& responseString)
{
#if 0
    QByteArray paramStart = request.getParameter(APIPARAM_START);
    QByteArray paramCount = request.getParameter(APIPARAM_COUNT);

    if (paramStart.length() == 0 || paramCount.length() == 0) {
        sendFailure(400, "Missing parameters", response);
        return;
    }
    int start = paramStart.toInt();
    int count = paramCount.toInt();
#endif
    QJsonArray grpConvoArrayValue;

    WickrConvoList *curConvos = WickrCore::WickrConvo::getConvoList();

    // TODO: Should there be a semaphore to control access to this?
    if (curConvos != NULL) {
        QList<QString> keys = curConvos->acquireKeyList();

        // Process each of the convos
        for (int cid=0; cid < keys.size(); cid++) {

            WickrCore::WickrConvo *currentConvo = (WickrCore::WickrConvo *)curConvos->value( keys.at(cid) );
            QJsonObject groupConvoValue = getGroupConvoInfo(currentConvo);
            if (!groupConvoValue.isEmpty())
                grpConvoArrayValue.append(groupConvoValue);
        }
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_GROUPCONVOS, grpConvoArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    responseString = byteArray;
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

int
WickrIOAPIInterface::numMessages()
{
    int numMsgs = 0;

    WickrConvoList *curConvos = WickrCore::WickrConvo::getConvoList();

    // TODO: Should there be a semaphore to control access to this?
    if (curConvos != NULL) {
        QList<QString> keys = curConvos->acquireKeyList();

        // Process each of the convos
        for (int cid=0; cid < keys.size(); cid++) {
            WickrCore::WickrConvo *currentConvo = (WickrCore::WickrConvo *)curConvos->value( keys.at(cid) );
            WickrMessageList *themessages = currentConvo->getMessages();

            QList<QString> keys = themessages->acquireKeyList();

            for( int i=0; i<keys.size(); i++) {
                WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)themessages->value(keys.at(i));

                if( !msg )
                    continue;

                // Only handle inbox messages
                if( !msg->isInbox() )
                    continue;

#if 0
                // Unlock the message
                WickrStatus msgstatus = msg->unlock();
                if( msgstatus.isError() ) {
                    continue;
                }
#endif

                WickrCore::WickrInbox *inbox = (WickrCore::WickrInbox *)msg;

                if (inbox->getInboxState() == WICKR_INBOX_OPENED) {
                    WickrMsgClass mclass = msg->getMsgClass();
                    if( mclass == MsgClass_Text ) {
                        numMsgs++;
                    }
                }
            }
        }
    }
    return numMsgs;
}

bool
WickrIOAPIInterface::getStatistics(const QString& apiKey, QString& responseString)
{
    QJsonObject msgValues;
    QJsonObject statValues;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db != NULL) {
        statValues.insert(APIJSON_STATID_MSGCNT, numMessages());
        WickrBotClients *client;
        if (apiKey.isEmpty()) {
            client = m_operation->m_client;
        } else {
            client = db->getClientUsingApiKey(apiKey);
        }
        if (client != NULL) {
            statValues.insert(APIJSON_STATID_PENDING, db->getClientsActionCount(client->id));
            statValues.insert(APIJSON_STATID_PNDCBOUT, db->getClientsOutMessagesCount(client->id));

            QList<WickrBotStatistics *> stats;
            stats = db->getClientStatistics(client->id);
            if (stats.length() > 0) {
                for (WickrBotStatistics *stat : stats) {
                    switch (stat->statID) {
                    case DB_STATID_MSGS_TX:
                        statValues.insert(APIJSON_STATID_MSGSTX, stat->statValue);
                        break;
                    case DB_STATID_MSGS_RX:
                        statValues.insert(APIJSON_STATID_MSGSRX, stat->statValue);
                        break;
                    case DB_STATID_ERRORS_TX:
                        statValues.insert(APIJSON_STATID_ERRSTX, stat->statValue);
                        break;
                    case DB_STATID_ERRORS_RX:
                        statValues.insert(APIJSON_STATID_ERRSRX, stat->statValue);
                        break;
                    case DB_STATID_MSGS_OBOXSYNC:
                        statValues.insert(APIJSON_STATID_OBOXSYNC, stat->statValue);
                        break;
                    }
                }
            }
            msgValues.insert(APIJSON_STATISTICS, statValues);
        }
    }

    QJsonDocument saveDoc(msgValues);
    QByteArray byteArray = saveDoc.toJson();

    responseString = byteArray;
    return true;
}


bool
WickrIOAPIInterface::clearStatistics(const QString& apiKey, QString& responseString)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db != NULL) {
        WickrBotClients *client;
        if (apiKey.isEmpty()) {
            client = m_operation->m_client;
        } else {
            client = db->getClientUsingApiKey(apiKey);
        }

        if (client != NULL) {
            db->deleteClientStatistics(client->id);
            return true;
        } else {
            responseString = "Failed to find client with input API Key";
        }
    } else {
        responseString = "Problem handling with database";
    }
    return false;
}
