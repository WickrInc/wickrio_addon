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

#if 0   // Ignore authentication for now
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
#endif

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
    QByteArray paramCount = request.getParameter(APIPARAM_COUNT);
    int count;

    // Validate the parameters
    if (paramCount.length() != 0) {
        count = paramCount.toInt();
        if (count <= 0) {
            sendFailure(400, "Invalid count value", response);
            return;
        }
    } else {
        count = 1;
    }

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        sendFailure(500, "Internal Error: Failed to cast database!", response);
        return;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);
    QJsonArray messageArrayValue;

    // Make sure the start is less than the number of IDs retrieved
    if (msgIDs.size() == 0) {
    } else {
        int pos=0;
        while (pos < count) {
            // If there are no more messages then we are done
            if (pos >= msgIDs.count())
                break;

            WickrIOMessage rxMsg;
            if (db->getMessage(msgIDs.at(pos), &rxMsg)) {
                QJsonObject messageValue;
                QJsonDocument doc = QJsonDocument::fromJson(rxMsg.json.toUtf8());

                // check validity of the document
                if (!doc.isNull()) {
                    if(doc.isObject()) {
                        messageValue = doc.object();
                        pos++;
                        messageArrayValue.append(messageValue);
                    } else {
                        qDebug() << "Document is not an object!";
                    }
                } else {
                    qDebug() << "Invalid JSONn";
                }

                db->deleteMessage(rxMsg.id, true);
            } else {
                break;
            }
        }
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

