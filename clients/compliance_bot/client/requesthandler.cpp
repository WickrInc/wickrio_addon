
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
    if (group.toLower() == APIURL_MSGRECVCBACK)  {
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

