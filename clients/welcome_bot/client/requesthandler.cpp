
#include "requesthandler.h"
#include "wickrbotjsondata.h"
#include "wickrbotipc.h"
#include "wickrioeclientmain.h"
#include "wickrioapi.h"

#include "libinterface/libwickrcore.h"
#include "messaging/wickrInbox.h"
#include "messaging/wickrGroupControl.h"
#include "common/wickrRuntime.h"

#include "perftest.h"

PerfTest * RequestHandler::perftests[10];
bool RequestHandler::perftestsetup = false;

RequestHandler::RequestHandler(OperationData *operation, QObject* parent) :
    WickrIOHttpRequestHdlr(parent),
    m_operation(operation)
{
    if (!perftestsetup) {
        for (int i=0; i<10; i++) {
            QString name = QString("Test %1").arg(i+1);
            perftests[i] = new PerfTest(name);
        }
    }

    m_operation->log_handler->log("RequestHandler: created");
}


RequestHandler::~RequestHandler() {
    for (int i=0; i<10; i++) {
        perftests[i]->print();
        perftests[i]->deleteLater();
    }
    m_operation->log_handler->log("RequestHandler: deleted");
}

/**
 * @brief RequestHandler::service
 * This function will parse the incoming HTTP Request.
 * There are 2 formats for the incoming HTTP request. The supported actions defined
 * below will include definitions for the 2 different formats:
 *
 * @param request
 * @param response
 */
void RequestHandler::service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response) {

    QByteArray path=request.getPath();
    QByteArray method=request.getMethod();
    QString apiKey("");

    m_operation->log_handler->log(QString("Controller: path=%1, method=%2").arg(QString(path)).arg(QString(method)));

    // Set a pointer to the WickrIO Database, in the parent class
    m_ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);

    // Handle the OPTIONS method
    if (method.toLower() == "options") {
        optionsResponse(request, response);
        return;
    }
    setupResponse(request, response);

    // Get the Console User from the database
    WickrIOConsoleUser cUser;

    // Validate the authentication
    if (! validateAuthentication(request, response, &cUser)) {
        sendFailure(401, "Authentication failed", response);
        return;
    } else {
        if (!cUser.isAdmin() && m_ioDB->getClientsConsoleID(m_operation->m_client->id) != cUser.id) {
            sendFailure(403, "Forbidden", response);
            return;
        }
    }

    // Parse the path
    QList<QByteArray> pathSplit = path.split('/');

    // Have to be at least 3 parameters (use 4 since the / at the beginning will add an empty param
    if (pathSplit.length() < 4) {
        sendFailure(400, "Missing parameters", response);
        return;
    }

    // Get rid of the blank first parameter
    pathSplit.removeFirst();

    // Parse out the first 3 parameters
    QString controller(pathSplit.at(0));
    apiKey = QString(pathSplit.at(1));
    QString group(pathSplit.at(2));

    if (controller.toLower() != APIURL_APPS) {
        sendFailure(400, "Unknown parameter", response);
        return;
    }

    // Check if the API Key is in the database
    if (!m_operation->validateApiKey(apiKey)) {
        sendFailure(403, "Invalid API Key", response);
        return;
    }

    QString clientID;
    // Check if there is a specific client ID
    if (pathSplit.length() > 3) {
        clientID = QString(pathSplit.at(3));
    }


    QString methodString(method.data());

    // Handle the messages group
    if (group.toLower() == APIURL_MESSAGES) {
        if (methodString.toLower() == "post") {
            processSendMessage(request, response);
        } else if (methodString.toLower() == "get") {
            processGetMessages(request, response);
        } else if (methodString.toLower() == "delete") {
            processDeleteMessages(response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_MSGRECVCBACK)  {
        if (methodString.toLower() == "post") {
            setMsgRecvCallback(apiKey, request, response);
        } else if (methodString.toLower() == "get") {
            getMsgRecvCallback(apiKey, response);
        } else if (methodString.toLower() == "delete") {
            deleteMsgRecvCallback(apiKey, response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_MSGRECVEMAIL)  {
        if (methodString.toLower() == "post") {
            setMsgRecvEmail(apiKey, request, response);
        } else if (methodString.toLower() == "get") {
            getMsgRecvEmail(apiKey, response);
        } else if (methodString.toLower() == "delete") {
            deleteMsgRecvEmail(apiKey, response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_ROOMS) {
        if (methodString.toLower() == "post") {
            processAddRoom(request, response);
        } else if (methodString.toLower() == "get") {
            processGetRooms(clientID, response);
        } else if (methodString.toLower() == "delete") {
            processDeleteRoom(clientID, response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_STATISTICS) {
        if (methodString.toLower() == "get") {
            getStatistics(apiKey, response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else {
        sendFailure(400, "Unknown resource", response);
        return;
    }

    for (int i=0; i<10; i++) {
        perftests[i]->print();
    }

    m_operation->log_handler->log("RequestHandler: finished request");
}


/**
 * @brief RequestHandler::processSendMessage
 * This function will send the message associated with the input http request
 * @param request
 * @param response
 */
void
RequestHandler::processSendMessage(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray body=request.getBody();
    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);

    if (jsonHandler->parseSendMessage(body)) {
        m_operation->log_handler->log("Message parsed successfully");
        sendSuccess(response);
    } else {
        sendFailure(400, "Failed sending message", response);
        m_operation->log_handler->log("Message parsing failed!");
    }
    delete jsonHandler;
}

/**
 * @brief RequestHandler::processGetMessages
 * This function will process the "getmessages" GET command.
 * @param resultCode
 * @param start
 * @param count
 * @return
 */
void
RequestHandler::processGetMessages(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray paramStart = request.getParameter(APIPARAM_START);
    QByteArray paramCount = request.getParameter(APIPARAM_COUNT);

    // Validate the API Key
    if (paramStart.length() == 0 || paramCount.length() == 0) {
        sendFailure(400, "Missing parameters", response);
        return;
    }
    int start = paramStart.toInt();
    int count = paramCount.toInt();
    int messagesRecv = 0;
    QJsonArray messageArrayValue;

    WickrConvoList *curConvos = WickrCore::WickrConvo::getConvoList();

    // TODO: Should there be a semaphore to control access to this?
    if (curConvos != NULL) {
        QList<QString> keys = curConvos->acquireKeyList();

        // Process each of the convos
        for (int cid=0; cid < keys.size(); cid++) {
            perftests[0]->start("Per convo");

            WickrCore::WickrConvo *currentConvo = (WickrCore::WickrConvo *)curConvos->value( keys.at(cid) );
            WickrMessageList *themessages = currentConvo->getMessages();

#if 0
            long msgcount = themessages->size();

            m_operation->log_handler->log(QString("CurrentConvo: id %1, vgroupTag: %2, allusersstring: %3, timestamp %4")
                                    .arg(currentConvo->getID())
                                    .arg(currentConvo->getVGroupTag())
                                    .arg(currentConvo->getAllUsersString())
                                    .arg(currentConvo->getLastTimestampString()));
            m_operation->log_handler->log((QString("Msgs (%1), Unread (%2)")
                                      .arg(QString::number(msgcount))
                                     .arg(QString::number(currentConvo->getUnreadMessageCount()))));

            WickrCore::WickrUser *lastuser = currentConvo->getLastUser();
            if( lastuser ) {
                m_operation->log_handler->log(QString("primary name: %1").arg(lastuser->getPrimaryName()));
                m_operation->log_handler->log(QString("key ID Long: %1").arg(lastuser->getKeyIdentityLong()));
                m_operation->log_handler->log(QString("User ID Hash:").arg(lastuser->getUserIDHash()));
            }
#endif
perftests[1]->start("acquireKeylist");
            QList<QString> keys = themessages->acquireKeyList();
perftests[1]->stop();

            for( int i=0; i<keys.size(); i++) {
                WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)themessages->value(keys.at(i));

                if( !msg )
                    continue;

                // Only handle inbox messages
                if( !msg->isInbox() )
                    continue;

perftests[4]->start("Message handling");

                // Unlock the message
//perftests[2]->start("unlock");
//                WickrStatus msgstatus = msg->unlock();
//perftests[2]->stop();
//                if( msgstatus.isError() ) {
//                    QString errorString = QString("Message id %1 failed: %2").arg(msg->getID()).arg(msgstatus.getValue());
//                    continue;
//                }

                QString inboxState;

                WickrCore::WickrInbox *inbox = (WickrCore::WickrInbox *)msg;
//                inbox->isUnlockRequired();

                if (inbox->getInboxState() == WICKR_INBOX_OPENED) {
                    WickrMsgClass mclass = msg->getMsgClass();
                    if( mclass == MsgClass_Text ) {
                        QJsonObject messageValue;

perftests[3]->start("getCachedText");
                        QString txt = msg->getCachedText();
perftests[3]->stop();
//                        qDebug() << txt;

                        if (messagesRecv >= start && messagesRecv < (start+count)) {
                            messageValue.insert(APIJSON_MSGID, messagesRecv);
                            messageValue.insert(APIJSON_MSGTEXT, txt);

                            // Get the sender of this message
                            WickrCore::WickrUser *sender = inbox->getSenderUser();
                            messageValue.insert(APIJSON_MSGSENDER, sender->getUserID());

                            // Setup the users array
                            QList<WickrCore::WickrUser *>users = currentConvo->getAllUsers();
                            QJsonArray usersArray;

                            for (WickrCore::WickrUser *user : users) {
                                QJsonObject userEntry;

                                userEntry.insert(APIJSON_NAME, user->getUserID());
                                usersArray.append(userEntry);
                            }
                            QJsonObject myUserEntry;
                            myUserEntry.insert(APIJSON_NAME, m_operation->m_client->user);
                            usersArray.append(myUserEntry);

                            messageValue.insert(APIJSON_MSGUSERS, usersArray);

                            // Message timestamp
                            long timestamp = msg->getMsgTimestamp();
                            QDateTime msgDate;
                            msgDate.setMSecsSinceEpoch((quint64)timestamp * 1000l);
                            // get the "hh:mm ap" format based on locale
                            QString statusText = msgDate.toString( QLocale::system().dateTimeFormat(QLocale::ShortFormat));
                            messageValue.insert(APIJSON_MSGTIME, statusText);

                            // Message micro seconds
                            long usec = msg->getMsgUsec();
                            QString msg_ts = QString("%1.%2").arg(timestamp).arg(usec);
                            messageValue.insert(APIJSON_MSG_TS, msg_ts);

                            messageArrayValue.append(messageValue);
                        }

#if 0
                        if( msg->hasAttachments() ) {
                            QList<WickrCore::WickrAttachment*> attachments = msg->getAttachments();
                            qDebug() << "Message has" << attachments.size() << "attachments";
                            for( int i=0; i < attachments.size(); i++ ) {
                                WickrCore::WickrAttachment *att = attachments.at(i);
                                qDebug() << i << ": id" << att->getID() << "type" << att->getType() << "bytes" <<att->getData().length();
                            }
                        }
#endif
                    }

                    messagesRecv++;
                }
                perftests[4]->stop();

            }
        }

        perftests[0]->stop();
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_MESSAGES, messageArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

void
RequestHandler::processDeleteMessages(stefanfrings::HttpResponse& response)
{
    int messagesDeleted = 0;

    WickrConvoList *curConvos = WickrCore::WickrConvo::getConvoList();

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

                WickrCore::WickrInbox *inbox = (WickrCore::WickrInbox *)msg;

                inbox->dodelete(traceInfo());
//                delete inbox;

                messagesDeleted++;
            }
        }
    }

    sendSuccess(response);
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
RequestHandler::getJsonArrayValue(QJsonObject jsonObject, QString jsonName, QString jsonArray)
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

void
RequestHandler::processAddRoom(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray jsonString = request.getBody();

    // Get the room details from the incoming message
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(jsonString, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        sendFailure(400, "Failed", response);
        return;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    QJsonObject roomObject;

    // Start the operation
    if (jsonObject.contains(APIJSON_ROOM)) {
        value = jsonObject[APIJSON_ROOM];
        roomObject = value.toObject();
    } else {
        sendFailure(400, "malformed JSON", response);
        return;
    }

    // Check that required objects exist
    if (!roomObject.contains(APIJSON_ROOMMEMBERS) || !roomObject.contains(APIJSON_ROOMMASTERS) ||
        !roomObject.contains(APIJSON_ROOMTITLE))
    {
        sendFailure(400, "Missing required data", response);
        return;
    }

    QStringList memberslist;
    QStringList masterslist;
    QString title;
    QString description;
    int ttl = 0;

    // Get the TTL / Destruct time
    if (roomObject.contains(APIJSON_ROOMTTL)) {
        value = roomObject[APIJSON_ROOMTTL];
        ttl = value.toInt(0);
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
        sendFailure(400, "Invalid title", response);
        return;
    }

    // Get the members
    memberslist = getJsonArrayValue(roomObject, APIJSON_NAME, APIJSON_ROOMMEMBERS);
    QString members = memberslist.join(",");
    if (members.isEmpty()) {
        sendFailure(400, "Invalid members list", response);
        return;
    }

    QStringList memberHashes;
    foreach (QString member, memberslist) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(member);
        if (user == NULL) {
            sendFailure(400, "Cannot find user record for member", response);
            return;
        }
        QString idHash = user->getServerIDHash();
        if (idHash.isEmpty()) {
            sendFailure(500, "Problem handling user record for member", response);
            return;
        }
        memberHashes.append(idHash);
    }

    // Get the list of masters
    masterslist = getJsonArrayValue(roomObject, APIJSON_NAME, APIJSON_ROOMMASTERS);
    if (masterslist.size() == 0) {
        sendFailure(400, "Invalid masters list", response);
        return;
    }

    QStringList masterHashes;
    foreach (QString master, masterslist) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(master);
        if (user == NULL) {
            sendFailure(400, "Cannot find user record for master", response);
            return;
        }
        QString idHash = user->getServerIDHash();
        if (idHash.isEmpty()) {
            sendFailure(500, "Problem handling user record for master", response);
            return;
        }
        masterHashes.append(idHash);
    }

    // Create the room

    // Create convo (create if not found, no group name, secure room)
    QString mastersString = masterHashes.join(',');
    QString membersString = memberHashes.join(',');

#if 0
    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (roomMgr) {
        roomMgr->createSecureRoomConvoStart(membersString, mastersString, ttl, title, description);
    }

#endif
    sendSuccess(response);
}

void
RequestHandler::onCreateSecureRoom(const QString& vGroupID, const QStringList& mastersHashList, int destructionTime, const QString& roomTitle, const QString& roomDescription)
{
    qDebug() << "RequestHandler::onCreateSecureRoom called!!";
    WickrCore::WickrConvo* convo = WickrCore::WickrConvo::getConvoWithvGroupID( vGroupID );
    if ( convo ) {
        qDebug() << "CONTROL CREATE SECURE ROOM:"
                 << "\nROOM NAME      : " << roomTitle
                 << "\nROOM PURPOSE   : " << roomDescription
                 << "\nROOM MASTERS   : " << mastersHashList
                 << "\nROOM TTL       : " << destructionTime
                 << "\nROOM VGROUP-ID : " << vGroupID;

        // Update convo
        convo->setVGroupTag(roomTitle);
        convo->setDestruct(destructionTime);
        convo->setRoomPurpose(roomDescription);

        // Add room masters
        for (int i=0; i < mastersHashList.size(); i++) {
            WickrCore::WickrUser *user = WickrCore::WickrUser::getUserByServerID(mastersHashList.at(i));
            if (user)
            {
                convo->addMaster(user, WickrCore::WickrUser::getSelfUser());
            }
        }
        convo->save();
    }
}

/**
 * @brief RequestHandler::deleteConvo
 * Will delete local convo (secure room or 1-to-1). If secure room, will kick off
 * deleteSecureRoomStart service to delete remotes.
 *
 */
bool
RequestHandler::deleteConvo(bool isSecureConvo, const QString& vgroupID)
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
            // If 1-To-1 convo
            WICKRBOT->m_convoHdlr.deleteConvo(convo);
        }
    }
    return true;
}

void
RequestHandler::processDeleteRoom(const QString &clientID, stefanfrings::HttpResponse& response)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(clientID);
    if (convo) {
        if (!convo->getIsRoomMaster()) {
            sendFailure(400, "Must be room master", response);
        } else {
            if (RequestHandler::deleteConvo(true, clientID)) {
                sendSuccess(response);
            } else {
                sendFailure(400, "Failed to delete room", response);
            }
        }
    }
}

void
RequestHandler::processGetRooms(const QString &clientID, stefanfrings::HttpResponse& response)
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

            // If a secure room then add to the list
            if (WickrCore::WickrConvo::getConvoTypeFromVGroupID(currentConvo->getVGroupID()) == CONVO_SECURE_ROOM &&
                currentConvo->isSecureRoomStateSynced()) {
                QJsonObject roomValue;

                roomValue.insert(APIJSON_ROOMTITLE, currentConvo->getVGroupTag());
                roomValue.insert(APIJSON_ROOMDESC, currentConvo->getRoomPurpose());
                roomValue.insert(APIJSON_VGROUPID, currentConvo->getVGroupID());
                roomValue.insert(APIJSON_ROOMTTL, QString::number(currentConvo->getDestruct()));

                // Setup the users array
                QStringList masterslist = currentConvo->getRoomMastersUserName();
                QJsonArray mastersArray;

                for (QString master : masterslist) {
                    QJsonObject masterEntry;

                    masterEntry.insert(APIJSON_NAME, master);
                    mastersArray.append(masterEntry);
                }
                roomValue.insert(APIJSON_ROOMMASTERS, mastersArray);

                // Setup the users array
                QStringList userslist = currentConvo->getUsernameStringArray();
                QJsonArray usersArray;

                for (QString user : userslist) {
                    QJsonObject userEntry;

                    userEntry.insert(APIJSON_NAME, user);
                    usersArray.append(userEntry);
                }
                roomValue.insert(APIJSON_ROOMMEMBERS, usersArray);


                roomArrayValue.append(roomValue);
            }
        }
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_ROOMS, roomArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

int
RequestHandler::numMessages()
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

void
RequestHandler::getStatistics(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    QJsonObject msgValues;
    QJsonObject statValues;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db != NULL) {
        statValues.insert(APIJSON_STATID_MSGCNT, numMessages());
        WickrBotClients *client = db->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            statValues.insert(APIJSON_STATID_PENDING, db->getClientsActionCount(client->id));

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
                    }
                }
            }
            msgValues.insert(APIJSON_STATISTICS, statValues);
        }
    }

    QJsonDocument saveDoc(msgValues);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

