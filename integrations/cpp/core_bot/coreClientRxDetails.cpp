#include <QJsonArray>
#include <QJsonDocument>

#include "coreClientRxDetails.h"
#include "wickrioapi.h"

#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
//#include "wickrIOServerCommon.h"

CoreClientRxDetails::CoreClientRxDetails(CoreOperationData *operation) :
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

    //TODO: Process the JSON
//    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);

    QStringList commands;
    QStringList raw;
    QString     response;
    bool        sendFile = false;
    QString     file2send;

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
                response = "client list\nclient [getoutput|getlog|start|pause|statistics] <name>\nservice [status|start|stop]\nintegrations list\n";
            } else {
                response = "client list\nclient [getoutput|getlog|start|pause|statistics] <name>\nintegrations list\n";
            }
        } else if (commands[0] == "client") {
            if (commands.count() >= 2) {
                if (commands[1] == "list") {
                    response = getClientList();
                } else if (commands[1] == "getoutput") {
                    if (commands.count() == 3) {
                        QString fname = this->getClientFile(raw[2], CMD_CLIENTFILE_OUTPUT);
                        if (fname.isEmpty()) {
                            response = "Cannot find output file";
                        } else {
                            sendFile = true;
                            file2send = fname;
                            response = "send file";
                        }
                    }
                } else if (commands[1] == "getlog") {
                    if (commands.count() == 3) {
                        QString fname = getClientFile(raw[2], CMD_CLIENTFILE_LOG);
                        if (fname.isEmpty()) {
                            response = "Cannot find log file";
                        } else {
                            sendFile = true;
                            file2send = fname;
                            response = "send log file";
                        }
                    } else {
                        response = "Invalid command: missing bot name";
                    }
                } else if (commands[1] == "statistics") {
                    response = getStats(raw[2]);
                } else {
                    response = "Invalid command";
                }
            } else {
                response = "Invalid command";
            }
        } else {
            response = "Invalid command";
        }
    }

    // TODO: send the reponse
    // TODO: if there is a file to send then send it

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
    if (db == nullptr) {
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
    if (db == nullptr) {
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
        QString outputFileName = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(clientName);
        QFileInfo fi(outputFileName);
        if (fi.exists()) {
            filename = outputFileName;
        }

        if (filename.isEmpty()) {
            QString configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(clientName);

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
        QString logFileName = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(clientName);
        QFileInfo fi(logFileName);
        if (fi.exists()) {
            filename = logFileName;
        }

        if (filename.isEmpty()) {
            QString configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(clientName);

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
CoreClientRxDetails::getStats(const QString& clientName)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == nullptr) {
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
