
#include "requesthandler.h"
#include "wickrbotjsondata.h"
#include "wickrbotipc.h"
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
    m_operation(operation),
    m_apiInterface(operation, parent)
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
            if (clientID.isEmpty()) {
                processGetGroupConvos(response);
            } else {
                processGetGroupConvo(clientID, response);
            }
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


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

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
    QString retString;

    if (!m_apiInterface.sendMessage(body, retString)) {
        sendFailure(400, retString.toLatin1(), response);
    } else {
        sendSuccess(response);
    }
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

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void
RequestHandler::processAddRoom(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray jsonString = request.getBody();
    QString responseString;

    if (m_apiInterface.addRoom(jsonString, responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processUpdateRoom(const QString &vGroupID, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray jsonString = request.getBody();
    QString responseString;

    if (m_apiInterface.updateRoom(vGroupID, jsonString, responseString)) {
        sendSuccess(response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
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

void
RequestHandler::processDeleteRoom(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.deleteRoom(vGroupID, responseString)) {
        sendSuccess(response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processLeaveRoom(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.leaveRoom(vGroupID, responseString)) {
        sendSuccess(response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processGetRoom(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.getRoom(vGroupID, responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processGetRooms(stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.getRooms(responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void
RequestHandler::processAddGroupConvo(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray jsonString = request.getBody();
    QString responseString;

    if (m_apiInterface.addGroupConvo(jsonString, responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processDeleteGroupConvo(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.deleteGroupConvo(vGroupID, responseString)) {
        sendSuccess(response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processGetGroupConvo(const QString &vGroupID, stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.getGroupConvo(vGroupID, responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

void
RequestHandler::processGetGroupConvos(stefanfrings::HttpResponse& response)
{
    QString responseString;

    if (m_apiInterface.getGroupConvos(responseString)) {
        sendSuccess(responseString.toLatin1(), response);
    } else {
        sendFailure(400, responseString.toLatin1(), response);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void
RequestHandler::getStatistics(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    QString retString;

    if (!m_apiInterface.getStatistics(apiKey, retString)) {
        sendFailure(400, retString.toLatin1(), response);
    } else {
        sendSuccess(retString.toLatin1(), response);
    }
}

void
RequestHandler::clearStatistics(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    QString retString;

    if (!m_apiInterface.clearStatistics(apiKey, retString)) {
        sendFailure(400, retString.toLatin1(), response);
    } else {
        sendSuccess(response);
    }
}

