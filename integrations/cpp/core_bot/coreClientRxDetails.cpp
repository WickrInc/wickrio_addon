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

#include "wickrbotjsondata.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "wickrIOServerCommon.h"
#include "wickrIOConsoleClientHandler.h"
#include "consoleserver.h"
#include "wickrIOSendMessageState.h"
#include "wickrIOAddClientState.h"

CoreClientRxDetails::CoreClientRxDetails(OperationData *operation) :
    m_operation(operation)
{
}

CoreClientRxDetails::~CoreClientRxDetails()
{
}


bool CoreClientRxDetails::processMessage(const QString& txt)
{
    if (m_operation == nullptr)
        return false;

    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == nullptr) {
        return false;
    }

    m_processing = true;
    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);

    QDateTime dt = QDateTime::currentDateTime();
    jsonHandler->m_runTime = dt;

    // TODO: Get client security level

    QStringList commands;
    QStringList raw;

    // See if there is a command state for this user
    WickrIOCmdState *cmdState = getCmdState(m_userid);

    if (cmdState != nullptr) {
        commands = cmdState->originalCommand().toLower().split(" ");
        raw = cmdState->originalCommand().split(" ");
    } else {
        commands = txt.toLower().split(" ");
        raw = txt.split(" ");
    }

    if (commands.count() == 0) {
        qDebug() << "Got command with 0 arguments";
    } else {
        if (commands[0] == "help") {
            if (m_user.isAdmin()) {
                jsonHandler->m_message = "client list\nclient [getoutput|getlog|start|pause|statistics] <name>\nservice [status|start|stop]\nintegrations list\n";
            } else {
                jsonHandler->m_message = "client list\nclient [getoutput|getlog|start|pause|statistics] <name>\nintegrations list\n";
            }
        } else if (commands[0] == "integrations") {
            if (commands.count() >= 2) {
                if (commands[1] == "list") {
                    jsonHandler->m_message = getIntegrationsList();
                } else {
                    jsonHandler->m_message = "Invalid command: missing invalid options";
                }
            } else {
                jsonHandler->m_message = "Invalid command: missing integrations option";
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
                } else if (commands[1] == "statistics") {
                    jsonHandler->m_message = getStats(raw[2]);
                } else if (commands[1] == "add") {
                    jsonHandler->m_message = addClient(txt, cmdState);
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
                    QString clientState = WickrIOConsoleClientHandler::getActualProcessState(WBIO_CLIENTSERVER_TARGET, WBIO_CLIENTSERVER_TARGET, ioDB);
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

    // If the command is done then free the associated memory
    if (cmdState && cmdState->done()) {
        deleteCmdState(m_userid);
        delete cmdState;
    }
    return true;
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

QString CoreClientRxDetails::addClient(const QString& txt, WickrIOCmdState *cmdState)
{
    WickrIOAddClientState *cs;

    if (cmdState) {
        cs = static_cast<WickrIOAddClientState *>(cmdState);
        if (cs == nullptr) {
            cmdState->setDone();
            return "Internal failure!";
        }

        cs->m_process.write(txt.toLatin1());
    } else {
        cs = new WickrIOAddClientState(m_userid, txt);
        cs->setCommand(0);

        QString command = "WickrIOConsoleCmd \"client,add\"";

        cs->m_process.setProcessChannelMode(QProcess::MergedChannels);
        cs->m_process.start(command, QIODevice::ReadWrite);
        addCmdState(m_userid, cs);
    }

    if (!cs->m_process.waitForStarted()) {
        delete cs;
        return "Failed to start create client software!";
    }

    bool gotaline = false;
    while (cs->m_process.waitForReadyRead(-1)) {
        while (cs->m_process.canReadLine()) {
            QString bytes = QString(cs->m_process.readLine());
            cs->setOutput(bytes);
            gotaline = true;
            break;
        }
        if (gotaline)
            break;
    }

    return cs->output();
}


QString
CoreClientRxDetails::getStats(const QString& clientName)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return "Internal error";
    }

    WickrBotClients *client = db->getClientUsingName(clientName);
    if (client == nullptr) {
        return "Cannot find client!";
    }

    QString statValues;
    statValues += QString("Pending Messages %1").arg(db->getClientsActionCount(client->id));

    QList<WickrBotStatistics *> stats;
    stats = db->getClientStatistics(client->id);
    if (stats.length() > 0) {
        for (WickrBotStatistics *stat : stats) {
            switch (stat->statID) {
            case DB_STATID_MSGS_TX:
                statValues += QString("\nTx messages %1").arg(stat->statValue);
                break;
            case DB_STATID_MSGS_RX:
                statValues += QString("\nRx messages %1").arg(stat->statValue);
                break;
            case DB_STATID_ERRORS_TX:
                statValues += QString("\nTx errors %1").arg(stat->statValue);
                break;
            case DB_STATID_ERRORS_RX:
                statValues += QString("\nRx errors %1").arg(stat->statValue);
                break;
            }
        }
    }
    return statValues;
}

QString
CoreClientRxDetails::getIntegrationsList()
{
    QStringList integrationList;
    QString fileName = QString("%1/integrations/integrations.json").arg(WBIO_DEFAULT_DBLOCATION);
    QFile integrations(fileName);
    if( integrations.exists() ) {
        integrations.open( QFile::ReadOnly );
        QByteArray integrationJson = integrations.readAll();
        integrations.close();

        QJsonDocument d;
        d = d.fromJson(integrationJson);
        QJsonObject jsonObject = d.object();

        if (jsonObject.contains("integrations")) {
            QJsonArray integrationsArray = jsonObject.value("integrations").toArray();
            for (int i=0; i<integrationsArray.size(); i++) {
                QJsonValue arrayValue;
                arrayValue = integrationsArray[i];

                if (arrayValue.isObject()) {
                    // Get the title for this contact entry
                    QJsonObject arrayObject = arrayValue.toObject();

                    if (arrayObject.contains("name")) {
                        QJsonValue nameObj = arrayObject["name"];
                        QString name = nameObj.toString();
                        QJsonValue existsObj = arrayObject["exists"];
                        QString exists = existsObj.toString();
                        if (exists.toLower() == "true") {
                            integrationList.append(name);
                        }
                    }
                }
            }
        }
    }

    QString integrationString = integrationList.join("\n");
    QString retString;
    if (integrationString.isEmpty()) {
        retString = "There are no integrations currently available!";
    } else {
        retString = QString("Available integrations:\n%1").arg(integrationString);
    }
    return retString;
}


#include "createjson.h"

/**
 * @brief CoreClientRxDetails::getCmdState
 * Get the current command state record for the input user id.
 * @param userid
 * @return
 */
WickrIOCmdState *CoreClientRxDetails::getCmdState(const QString& userid)
{
    if (m_cmdState.contains(userid)) {
        return m_cmdState[userid];
    }
    return nullptr;
}

/**
 * @brief CoreClientRxDetails::addCmdState
 * Put the input command state record in the command state table.  If there is an
 * existing entry it will be removed and the new one inserted.
 * @param userid
 * @param state
 * @return
 */
bool CoreClientRxDetails::addCmdState(const QString& userid, WickrIOCmdState *state)
{
    if (m_cmdState.contains(userid)) {
        m_cmdState.remove(userid);
    }
    m_cmdState.insert(userid, state);
    return true;
}

/**
 * @brief CoreClientRxDetails::deleteCmdState
 * Remove a command state value for the input user
 * @param userid
 * @return
 */
bool CoreClientRxDetails::deleteCmdState(const QString& userid)
{
    if (m_cmdState.contains(userid)) {
        m_cmdState.remove(userid);
    }
    return true;
}
