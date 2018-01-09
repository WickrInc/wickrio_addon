#include <QJsonArray>
#include <QJsonDocument>

#include "coreClientRxDetails.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"

#include "common/wickrNotifyList.h"

#include "wickrIOClientRuntime.h"

#include "wickrbotjsondata.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "wickrIOServerCommon.h"
#include "wickrIOConsoleClientHandler.h"
#include "consoleserver.h"

CoreClientRxDetails::CoreClientRxDetails(OperationData *operation) : WickrIORxDetails(operation)
{
}

CoreClientRxDetails::~CoreClientRxDetails()
{
}

bool CoreClientRxDetails::init()
{
    return true;
}

bool CoreClientRxDetails::healthCheck()
{
    return true;
}

bool CoreClientRxDetails::processMessage(WickrDBObject *item)
{
    bool deleteMsg = false;

    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return false;
    }

    m_processing = true;
    WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;

    // If this is an outbox message then return (drop the message)
    if (msg && !msg->isInbox()) {
        qDebug() << "slotProcessMessage: have an outbox message, to drop!";
        m_processing = false;

        WickrCore::WickrOutbox *outbox = (WickrCore::WickrOutbox *)msg;

        if (outbox->isDeleted()) {
            return false;
        } else {
#if 1
            return false;
#else
            return true;
#endif
        }
    }

    qDebug() << "slotProcessMessage: have a message to be processed!";

    if (msg && msg->isInbox()) {
        QString inboxState;
        bool gotInboxMsg = false;
        m_messagesRecv++;
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_MSGS_RX, DB_STATDESC_TOTAL, 1);
        }

        // Only one to one messages are supported
        if (msg->getConvo()->getConvoType() != CONVO_ONE_TO_ONE) {
            m_messagesRecvInvalid++;
            return true;
        }

        WickrCore::WickrInbox *inbox = (WickrCore::WickrInbox *)msg;
        switch( inbox->getInboxState() ) {
        case WICKR_INBOX_NEEDS_DOWNLOAD:
            inboxState = "NEEDS_DOWNLOAD";
            break;
        case WICKR_INBOX_OPENED:
            inboxState = "OPENED";
            gotInboxMsg = true;
            break;
        case WICKR_INBOX_EXPIRED:
            inboxState = "EXPIRED";
            break;
        case WICKR_INBOX_DELETED:
            inboxState = "DELETED";
            break;
        }

        if( gotInboxMsg ) {
            bool processMsg = true;
            QString txt;

            WickrCore::WickrUser *sender = msg->getSenderUser();
            QString userid;
            if (sender != nullptr) {
                userid = sender->getUserID();
            } else {

            }

            // If we do not know the user id or there is no console user then don't respond
            if (userid.isEmpty() || !db->getUser(userid, m_operation->m_client->id, &m_user)) {
                if (userid.isEmpty()) {
                    qDebug() << "Sender's userid NOT found!";
                } else {
                    qDebug() << "No user records found for " << userid;
                }
                return true;
            } else {
                qDebug() << "Sender's userid=" << userid;
            }


            WickrMsgClass mclass = msg->getMsgClass();
            if( mclass == MsgClass_Text ) {
                txt = msg->getCachedText();
                deleteMsg = true;
            } else {
                processMsg = false;
            }

            if (processMsg) {
                WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);

                QDateTime dt = QDateTime::currentDateTime();
                jsonHandler->m_runTime = dt;
                    msg->getSenderUserID();
                if (msg->getConvo()->getConvoType() == CONVO_ONE_TO_ONE) {
                    jsonHandler->m_vgroupid = QString("");
                    QList<WickrCore::WickrUser *> users = msg->getConvo()->getAllUsers();
                    if (users.count() > 0 && ! users.at(0)->getUserID().isEmpty()) {
                        jsonHandler->m_userNames.append(users.at(0)->getUserID());
                        jsonHandler->m_userIDs.clear();
                    } else {
                        jsonHandler->m_userNames.clear();
                        jsonHandler->m_userIDs.append(msg->getSenderUserID());
                    }
                } else {
                    jsonHandler->m_userNames.clear();
                    jsonHandler->m_userIDs.clear();
                    jsonHandler->m_vgroupid = inbox->getConvo()->getVGroupID();
                }

                // TODO: Get client security level

                QStringList commands = txt.toLower().split(" ");
                QStringList raw = txt.split(" ");

                if (commands.count() == 0) {
                    qDebug() << "Got command with 0 arguments";
                } else {
                    if (commands[0] == "help") {
                        if (m_user.isAdmin()) {
                            jsonHandler->m_message = "client list\nclient [getoutput|getlog|start|pause] <name>\nservice [status|start|stop]\n";
                        } else {
                            jsonHandler->m_message = "client list\nclient [getoutput|getlog|start|pause] <name>\n";
                        }
                    } else if (commands[0] == "client") {
                        if (commands.count() >= 2) {
                            if (commands[1] == "list") {
                                jsonHandler->m_message = getClientList();
                            } else if (commands[1] == "getoutput") {
                                if (commands.count() == 3) {
                                    QString fname = this->getClientFile(raw[2], CMD_CLIENTFILE_OUTPUT);
                                    if (fname.isEmpty()) {
                                        jsonHandler->m_message = "Cannot find output file";
                                    } else {
                                        jsonHandler->m_message = "send file";
                                        jsonHandler->m_attachments.append(fname);

                                        QString cacheFilename = m_operation->m_botDB->getAttachment(fname);
                                        if (cacheFilename.isEmpty()) {
                                            QFileInfo fi(fname);
                                            m_operation->m_botDB->insertAttachment(fname, fname, (int)fi.size());
                                        }
                                    }
                                }
                            } else if (commands[1] == "getlog") {
                                if (commands.count() == 3) {
                                    QString fname = getClientFile(raw[2], CMD_CLIENTFILE_LOG);
                                    if (fname.isEmpty()) {
                                        jsonHandler->m_message = "Cannot find log file";
                                    } else {
                                        jsonHandler->m_message = "send log file";
                                        jsonHandler->m_attachments.append(fname);

                                        QString cacheFilename = m_operation->m_botDB->getAttachment(fname);
                                        if (cacheFilename.isEmpty()) {
                                            QFileInfo fi(fname);
                                            m_operation->m_botDB->insertAttachment(fname, fname, (int)fi.size());
                                        }
                                    }
                                } else {
                                    jsonHandler->m_message = "Invalid command: missing bot name";
                                }
                            } else if (commands[1] == "pause") {
                                if (commands.count() == 3) {
                                    jsonHandler->m_message = pauseClient(raw[2]);
                                } else {
                                    jsonHandler->m_message = "Invalid command: missing bot name";
                                }
                            } else if (commands[1] == "start") {
                                if (commands.count() == 3) {
                                    jsonHandler->m_message = startClient(raw[2]);
                                } else {
                                    jsonHandler->m_message = "Invalid command: missing bot name";
                                }
                            } else {
                                jsonHandler->m_message = "Invalid command";
                            }
                        } else {
                            jsonHandler->m_message = "Invalid command";
                        }
                    } else if (commands[0] == "service") {
                        if (m_user.isAdmin() && commands.count() >= 2) {
                            if (commands[1] == "status") {
                                WickrIOClientDatabase* ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
                                QString clientState = WickrIOConsoleClientHandler::getActualProcessState(WBIO_CLIENTSERVER_TARGET, ioDB);
                                jsonHandler->m_message = clientState;
                            } else if (commands[1] == "start") {
                                WickrIOClientDatabase* ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
                                ConsoleServer consoleServer(ioDB);
                                QString status = consoleServer.setState(true, WBIO_CLIENTSERVER_TARGET);
                                jsonHandler->m_message = status;
                            } else if (commands[1] == "stop") {
                                WickrIOClientDatabase* ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
                                ConsoleServer consoleServer(ioDB);
                                QString status = consoleServer.setState(false, WBIO_CLIENTSERVER_TARGET);
                                jsonHandler->m_message = status;
                            }
                        } else {
                            jsonHandler->m_message = "Invalid command";
                        }
                    } else {
                        jsonHandler->m_message = "Invalid command";
                    }
                }

                jsonHandler->m_action = "sendmessage";
                if (!jsonHandler->postEntry4SendMessage()) {
                    qDebug() << "Failed to send message!";
                }
            }
        }
    }
    return deleteMsg;
}

QString
CoreClientRxDetails::getClientList()
{
    QList<WickrBotClients *> clients;
    QString clientsString;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return clientsString;
    }

    // Get the clients records.  If admin console user get all of the clients
    if (m_user.isAdmin()) {
        clients = db->getClients();
    } else {
        QList<int> clientIDs = db->getUserClients(m_user.m_id);
        for (int clientID : clientIDs) {
            WickrBotClients *client = db->getClient(clientID);
            clients.append(client);
        }
    }

    int index=0;
    while (clients.length() > 0) {
        WickrBotClients * client = clients.first();
        clients.removeFirst();

        // Get the process state for this client
        QString stateString;
        WickrBotProcessState state;
        QString processName = QString("%1.%2").arg(client->binary).arg(client->name);

        if (m_operation->m_botDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                stateString = APIJSON_STATE_RUNNING;
            } else if (state.state == PROCSTATE_DOWN) {
                stateString = APIJSON_STATE_DOWN;
            } else if (state.state == PROCSTATE_PAUSED) {
                stateString = APIJSON_STATE_PAUSED;
            } else {
                stateString = APIJSON_STATE_UNKNOWN;
            }
        } else {
            stateString = APIJSON_STATE_DOWN;
        }

        QString clientString = QString("%1: %2, %3\n")
                .arg(index++)
                .arg(client->name)
                .arg(stateString);

        clientsString += clientString;

        delete client;
    }

    return clientsString;
}

QString
CoreClientRxDetails::getClients()
{
    QList<WickrBotClients *> clients;
    QString clientsString;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return clientsString;
    }

    // Get the clients records.  If admin console user get all of the clients
    if (m_user.isAdmin()) {
        clients = db->getClients();
    } else {
        QList<int> clientIDs = db->getUserClients(m_user.m_id);
        for (int clientID : clientIDs) {
            WickrBotClients *client = db->getClient(clientID);
            clients.append(client);
        }
    }

    while (clients.length() > 0) {
        WickrBotClients * client = clients.first();
        clients.removeFirst();

        // Get the process state for this client
        QString stateString;
        WickrBotProcessState state;
        QString processName = QString("%1.%2").arg(client->binary).arg(client->name);

        if (m_operation->m_botDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                stateString = APIJSON_STATE_RUNNING;
            } else if (state.state == PROCSTATE_DOWN) {
                stateString = APIJSON_STATE_DOWN;
            } else if (state.state == PROCSTATE_PAUSED) {
                stateString = APIJSON_STATE_PAUSED;
            } else {
                stateString = APIJSON_STATE_UNKNOWN;
            }
        } else {
            stateString = APIJSON_STATE_DOWN;
        }

        QString clientString = QString("%1, %2, %3, %4, %5, %6, %7\n")
                .arg(client->name)
                .arg(client->apiKey)
                .arg(client->user)
                .arg(client->iface)
                .arg(client->port)
                .arg(client->getIfaceTypeStr())
                .arg(client->binary)
                .arg(stateString);

        clientsString += clientString;

        delete client;
    }

    return clientsString;
}

QString
CoreClientRxDetails::getClientFile(const QString& clientName, CoreClientFileType type)
{
    QString filename;

    if (type == CMD_CLIENTFILE_OUTPUT) {
        QString outputFileName = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(m_operation->databaseDir).arg(clientName);
        QFileInfo fi(outputFileName);
        if (fi.exists()) {
            filename = outputFileName;
        }

        if (filename.isEmpty()) {
            QString configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(m_operation->databaseDir).arg(clientName);

            QFileInfo   fi(configFileName);
            if (fi.exists()) {
                QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

                // Set the output file if it is set
                settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
                QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
                if (! curOutputFilename.isEmpty()) {
                    filename = curOutputFilename;
                }
                settings->endGroup();
            }
        }
    } else if (type == CMD_CLIENTFILE_LOG) {
        QString logFileName = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(m_operation->databaseDir).arg(clientName);
        QFileInfo fi(logFileName);
        if (fi.exists()) {
            filename = logFileName;
        }

        if (filename.isEmpty()) {
            QString configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(m_operation->databaseDir).arg(clientName);

            QFileInfo   fi(configFileName);
            if (fi.exists()) {
                QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

                // Set the output file if it is set
                settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
                QString curLogFilename = settings->value(WBSETTINGS_LOGGING_FILENAME, "").toString();
                if (! curLogFilename.isEmpty()) {
                    filename = curLogFilename;
                }
                settings->endGroup();
            }
        }
    }

    return filename;
}

QString
CoreClientRxDetails::pauseClient(const QString& clientName)
{
    return "To be implemented";
}

QString
CoreClientRxDetails::startClient(const QString& clientName)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return "Internal error";
    }

    WickrBotClients *client = db->getClientUsingName(clientName);
    if (client == nullptr) {
        return "Cannot find client!";
    }
    WickrBotProcessState state;
    QString processName = WBIOServerCommon::getClientProcessName(client);

    if (m_operation->m_botDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_PAUSED) {
            if (m_operation->m_botDB->updateProcessState(processName, 0, PROCSTATE_DOWN)) {
                return "Client started";
            }
            return "Error changing client state";
        } else if (state.state == PROCSTATE_DOWN){
            return "Client is in Down state. The WickrIO Client Server should change the state to running. If this is not happening, please check that the WickrIOSvr process is running!";
        } else if (state.state == PROCSTATE_RUNNING) {
            return "Client is already in the running state";
        }
    } else {
        return "Could not get the clients state!";
    }
}
