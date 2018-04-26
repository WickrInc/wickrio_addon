#include <QJsonArray>

#include "wickrIOAPIInterface.h"
#include "wickrIOErrorHandler.h"
#include "wickrbotjsondata.h"
#include "wickrioapi.h"

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


