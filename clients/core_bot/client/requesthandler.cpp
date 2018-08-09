#include <QJsonArray>

#include "requesthandler.h"
#include "wickrbotjsondata.h"
#include "wickrIOClientMain.h"
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
            // differentiate an add from and update
            if (clientID.isEmpty()) {
                processAddRoom(request, response);
            } else {
                processUpdateRoom(clientID, request, response);
            }
        } else if (methodString.toLower() == "get") {
            if (clientID.isEmpty()) {
                processGetRooms(response);
            } else {
                processGetRoom(clientID, response);
            }
        } else if (methodString.toLower() == "delete") {
            QByteArray reason=request.getParameter("reason");
            if (reason.isEmpty() || reason == "delete") {
                processDeleteRoom(clientID, response);
            } else if (reason == "leave") {
                processLeaveRoom(clientID, response);
            } else {
                sendFailure(400, "Unknown command", response);
            }
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_GROUPCONVO) {
        if (methodString.toLower() == "post") {
            processAddGroupConvo(request, response);
        } else if (methodString.toLower() == "get") {
            processGetGroupConvos(clientID, response);
        } else if (methodString.toLower() == "delete") {
            processDeleteGroupConvo(clientID, response);
        } else {
            sendFailure(400, "Unknown command", response);
        }
    } else if (group.toLower() == APIURL_STATISTICS) {
        if (methodString.toLower() == "get") {
            getStatistics(apiKey, response);
        } else if (methodString.toLower() == "delete") {
            clearStatistics(apiKey, response);
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

    if (! jsonHandler->parseJson4SendMessage(body)) {
        sendFailure(400, "Failed parsing message JSON", response);
        m_operation->log_handler->log("Message JSON parsing failed!");
    } else {
        QStringList userNames = jsonHandler->getUserNames();
        QString vGroupID = jsonHandler->getVGroupID();

        if (userNames.isEmpty() && vGroupID.isEmpty()) {
            sendFailure(400, "No users identified", response);
            m_operation->log_handler->log("No users identified!");
        } else if (!userNames.isEmpty()){
            // Update and validate the input list of usernames.  If false is returned then
            // there was an error processing the list, or the user was invalid!
            if (updateAndValidateMembers(response, userNames)) {
                if (jsonHandler->postEntry4SendMessage()) {
                    m_operation->log_handler->log("Message parsed successfully");
                    sendSuccess(response);
                } else {
                    sendFailure(400, "Failed sending message", response);
                    m_operation->log_handler->log("Message parsing failed!");
                }
            }
        } else {
            if (jsonHandler->postEntry4SendMessage()) {
                m_operation->log_handler->log("Message parsed successfully");
                sendSuccess(response);
            } else {
                sendFailure(400, "Failed sending message", response);
                m_operation->log_handler->log("Message parsing failed!");
            }
        }
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

bool
RequestHandler::updateAndValidateMembers(stefanfrings::HttpResponse& response, const QStringList& memberslist, QStringList *memberHashes)
{
    QStringList memberSearchList;
    foreach (QString member, memberslist) {
        WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(member);
        if (user == NULL) {
            memberSearchList.append(member);
            continue;
        }
        QString idHash = user->getServerIDHash();
        if (idHash.isEmpty()) {
            sendFailure(500, "Problem handling user record!", response);
            return false;
        }
        if (memberHashes != nullptr)
            memberHashes->append(idHash);
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
                    sendFailure(400, "Failure waiting for user verification", response);
                    return false;
                }
            }
        }

        // Now make sure that user records have been created for the searched members
        foreach (QString searchMember, memberSearchList) {
            WickrCore::WickrUser *user = WickrCore::WickrUser::getUserWithAlias(searchMember);
            if (user == NULL) {
                QString errMsg = "Cannot find user record for " + searchMember;
                sendFailure(400, errMsg.toLatin1(), response);
                return false;
            }
            if (memberHashes != nullptr) {
                QString idHash = user->getServerIDHash();
                if (idHash.isEmpty()) {
                    QString errMsg = "Problem handling user record for " + searchMember;
                    sendFailure(500, errMsg.toLatin1(), response);
                    return false;
                }
                memberHashes->append(idHash);
            }
        }
    }
    return true;
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

    // Update and validate the input list of members.  If false is returned then
    // there was an error processing the list, or the member was invalid!
    if (!updateAndValidateMembers(response, memberslist, &memberHashes)) {
        return;
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

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (!roomMgr) {
        sendFailure(400, "Failed to create room", response);
        return;
    }

    QString vgroupID = roomMgr->createSecureRoomConvoStart(membersString,
                                                           mastersString,
                                                           ttl,
                                                           title,
                                                           description,
                                                           bor,
                                                           true);
    if (vgroupID.isEmpty()) {
        sendFailure(400, "Failed to create room", response);
        return;
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
            sendFailure(400, "Failed to create room", response);
            return;
        }
    }

    // Return the vGroupID
    QString body = QString("{ \"vgroupid\" : \"%1\" }").arg(vgroupID);
    sendSuccess(body.toLatin1(), response);
}

void
RequestHandler::processUpdateRoom(const QString &vGroupID, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray jsonString = request.getBody();

    // Get the room details from the incoming message
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(jsonString, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        sendFailure(400, "Failed", response);
        return;
    }

    // Get the convo associated with the room to be changed
    WickrCore::WickrConvo* convo = WickrCore::WickrConvo::getConvoWithvGroupID( vGroupID );
    if (!convo) {
        sendFailure(400, "Convo does not exist", response);
        return;
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
            sendFailure(400, "Invalid title", response);
            return;
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
            sendFailure(400, "Invalid members list", response);
            return;
        }

        // Update and validate the input list of members.  If false is returned then
        // there was an error processing the list, or the member was invalid!
        if (!updateAndValidateMembers(response, memberslist, &memberHashes)) {
            return;
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
        if (!updateAndValidateMembers(response, addMembersList, &addMembersHashes)) {
            return;
        }
        // Update and validate the input list of members.  If false is returned then
        // there was an error processing the list, or the member was invalid!
        if (!updateAndValidateMembers(response, delMembersList, &delMembersHashes)) {
            return;
        }
    }

    // Get the list of masters
    has_masterslist = jsonObject.contains(APIJSON_ROOMMASTERS);
    if (has_masterslist) {
        masterslist = getJsonArrayValue(jsonObject, APIJSON_NAME, APIJSON_ROOMMASTERS);
        if (masterslist.size() == 0) {
            sendFailure(400, "Invalid masters list", response);
            return;
        }

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
    }

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (!roomMgr) {
        sendFailure(400, "Failed to create room", response);
        return;
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
        sendFailure(400, "Failed to modify room", response);
        return;
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
            sendFailure(400, "Failed to create room", response);
            return;
        }
    }

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
RequestHandler::processLeaveRoom(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo) {
        if (convo->getConvoType() == CONVO_SECURE_ROOM || convo->getConvoType() == CONVO_GROUP_CONVO) {
            WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
            if (roomMgr) {
                roomMgr->leaveSecureRoomStart(vGroupID);
                sendSuccess(response);
            } else {
                sendFailure(500, "Internal error", response);
            }
        } else {
            sendFailure(400, "Can only leave secure room or group convo", response);
        }
    } else {
        sendFailure(400, "Cannot find convo for vgroupid", response);
    }
}

QJsonObject
RequestHandler::getRoomInfo(WickrCore::WickrConvo *convo)
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

void
RequestHandler::processGetRoom(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(vGroupID);
    if (convo != nullptr) {
        QJsonObject roomValue = getRoomInfo(convo);
        if (roomValue.isEmpty()) {
            sendFailure(400, "Convo is not a secure room", response);
        } else {
            QJsonDocument saveDoc(roomValue);
            QByteArray byteArray = saveDoc.toJson();

            sendSuccess(byteArray, response);
        }
    } else {
        sendFailure(400, "Can not find convo", response);
    }
}

void
RequestHandler::processGetRooms(stefanfrings::HttpResponse& response)
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

    sendSuccess(byteArray, response);
}

void
RequestHandler::processAddGroupConvo(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
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

    QJsonObject grpConvoObject;

    // Start the operation
    if (jsonObject.contains(APIJSON_GROUPCONVO)) {
        value = jsonObject[APIJSON_GROUPCONVO];
        grpConvoObject = value.toObject();
    } else {
        sendFailure(400, "malformed JSON", response);
        return;
    }

    // Check that required objects exist
    if (!grpConvoObject.contains(APIJSON_ROOMMEMBERS))
    {
        sendFailure(400, "Missing required data", response);
        return;
    }

    QStringList memberslist;
    int ttl = 0;

    // Get the TTL / Destruct time
    if (grpConvoObject.contains(APIJSON_ROOMTTL)) {
        value = grpConvoObject[APIJSON_ROOMTTL];
        ttl = value.toInt(0);
    }

    // Get the members
    memberslist = getJsonArrayValue(grpConvoObject, APIJSON_NAME, APIJSON_ROOMMEMBERS);
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

    // Create the room

    // Create convo (create if not found, no group name, secure room)
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
RequestHandler::processDeleteGroupConvo(const QString &clientID, stefanfrings::HttpResponse& response)
{
    WickrCore::WickrConvo *convo = WickrCore::WickrConvo::getConvoWithvGroupID(clientID);
    if (convo) {
        if (RequestHandler::deleteConvo(true, clientID)) {
            sendSuccess(response);
        } else {
            sendFailure(400, "Failed to delete Group Convo", response);
        }
    }
}

void
RequestHandler::processGetGroupConvos(const QString &clientID, stefanfrings::HttpResponse& response)
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

            // If a secure room then add to the list
            if (WickrCore::WickrConvo::getConvoTypeFromVGroupID(currentConvo->getVGroupID()) == CONVO_GROUP_CONVO) {
                QJsonObject grpConvoValue;

                grpConvoValue.insert(APIJSON_VGROUPID, currentConvo->getVGroupID());
                grpConvoValue.insert(APIJSON_ROOMTTL, QString::number(currentConvo->getDestruct()));

                // Setup the users array
                QStringList userslist = currentConvo->getUsernameStringArray();
                QJsonArray usersArray;

                for (QString user : userslist) {
                    QJsonObject userEntry;

                    userEntry.insert(APIJSON_NAME, user);
                    usersArray.append(userEntry);
                }
                grpConvoValue.insert(APIJSON_ROOMMEMBERS, usersArray);


                grpConvoArrayValue.append(grpConvoValue);
            }
        }
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_ROOMS, grpConvoArrayValue);
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

void
RequestHandler::clearStatistics(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db != NULL) {
        WickrBotClients *client = db->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            db->deleteClientStatistics(client->id);
            sendSuccess(response);
        } else {
            sendFailure(400, "Failed to find client with input API Key", response);
        }
    } else {
        sendFailure(500, "Problem handling with database", response);
    }
}

