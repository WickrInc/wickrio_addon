#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "cmdhandler.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "wickrIOConsoleClientHandler.h"
#include "wickrIOIPCRuntime.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

/**
 * @brief CmdHandler::CmdHandler
 *
 * @param parent
 */
CmdHandler::CmdHandler(QSettings *settings, OperationData *operation, QObject *parent) :
    WickrIOHttpRequestHdlr(parent),
    m_operation(operation),
    m_settings(settings)
{
    m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    m_settings->endGroup();

    // Get the email settings so we can send authorizations
    m_settings->beginGroup(WBSETTINGS_EMAIL_HEADER);
    m_email.readFromSettings(m_settings);
    m_settings->endGroup();
}

/**
 * @brief CmdHandler::~CmdHandler
 * Destructor for the Command Handler class. Make sure that all memory is deallocated!
 */
CmdHandler::~CmdHandler() {
}

/**
 * @brief CmdHandler::service
 * This function will parse the incoming HTTP Request.
 * @param request
 * @param response
 */
void CmdHandler::service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response) {

    QByteArray path=request.getPath();
    QByteArray method=request.getMethod();
    QString clientID;

    m_operation->log_handler->log(QString("Controller: path=%1, method=%2").arg(QString(path)).arg(QString(method)));

    // Handle the OPTIONS method
    if (method.toLower() == "options") {
        optionsResponse(request, response);
        return;
    }
    setupResponse(request, response);

    m_ioDB = dynamic_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (m_ioDB == NULL) {
        sendFailure(500, "Database error", response);
        return;
    }

    // Get the Console User from the database
    WickrIOConsoleUser consoleUser;

    // Validate the authentication
    if (! validateAuthentication(request, response, &consoleUser)) {
        sendFailure(401, "Authentication failed", response);
        return;
    }

    // Parse the path
    QList<QByteArray> pathSplit = path.split('/');

    // Have to be at least 2 parameters (use 3 since the / at the beginning will add an empty param
    if (pathSplit.length() < 3) {
        sendFailure(400, "Invalid path", response);
        return;
    }

    // Get rid of the blank first parameter
    pathSplit.removeFirst();

    // Parse out the first 3 parameters
    QString controller(pathSplit.at(0));
    QString group(pathSplit.at(1));

    if (controller.toLower() != APIURL_CONTROLLER) {
        sendFailure(400, "Invalid path", response);
        return;
    }

    WickrIOConsoleReqType reqType;

    // Check that the group parameter is one of the supported (currently only clients)
    if (group.toLower() == APIURL_CLIENTS) {
        reqType = ReqTypeClient;
    } else if (group.toLower() == APIURL_USERS) {
        reqType = ReqTypeUser;
    } else if (group.toLower() == APIURL_STATISTICS) {
        reqType = ReqTypeStatistic;
    } else {
        sendFailure(400, "Invalid API Key", response);
        return;
    }

    // Check if there is a specific client ID
    if (pathSplit.length() > 2) {
        clientID = QString(pathSplit.at(2));
    }

    QString methodString(method.data());
    QByteArray body=request.getBody();

    m_operation->log_handler->log(QString("Controller: body=%1").arg(QString(body)));

    bool invalidCommand = false;

    if (reqType == ReqTypeClient) {
        if (clientID.isEmpty()) {
            if (methodString.toLower() == "get") {
                getClients(&consoleUser, response);
            } else if (methodString.toLower() == "post") {
                addClient(&consoleUser, request, response);
            } else {
                invalidCommand = true;
            }
        } else {
            if (methodString.toLower() == "get") {
                getClient(&consoleUser, clientID, response);
            } else if (methodString.toLower() == "put") {
                updateClient(&consoleUser, clientID, request, response);
            } else if (methodString.toLower() == "delete") {
                deleteClient(&consoleUser, clientID, response);
            } else {
                invalidCommand = true;
            }
        }
    } else if (reqType == ReqTypeUser) {
        // The Console user MUST be admin to perform these commands
        if (! consoleUser.isAdmin()) {
            sendFailure(403, "You do not have permission to perform this operation", response);
        } else if (clientID.isEmpty()) {
            if (methodString.toLower() == "get") {
                getUsers(response);
            } else if (methodString.toLower() == "post") {
                addUser(request, response);
            } else {
                invalidCommand = true;
            }
        } else {
            if (methodString.toLower() == "get") {
                getUser(clientID, response);
            } else if (methodString.toLower() == "put") {
                updateUser(clientID, request, response);
            } else if (methodString.toLower() == "delete") {
                deleteUser(clientID, response);
            } else {
                invalidCommand = true;
            }
        }
    } else if (reqType == ReqTypeStatistic) {
        if (clientID.isEmpty()) {
            getStatistics(response);
        } else {
            getStatistics(clientID, response);
        }
    } else {
        invalidCommand = true;
    }

    if (invalidCommand) {
        sendFailure(400, "Unknown command", response);
    }

    m_operation->log_handler->log("CmdHandler: finished request");
}

#include "SmtpMime"
void
CmdHandler::generateNewKey(WickrIOConsoleUser *cUser)
{
    if (m_ioDB == NULL) {
        return;
    }

    WickrIOTokens token;

    // Generate a new Token
    if (m_ioDB->getConsoleUserToken(cUser->id, &token)) {
        token.token = WickrIOTokens::getRandomString(TOKEN_LENGTH);
        cUser->token = token.token;
        if (!m_ioDB->updateToken(&token)) {
            qDebug() << "Cannot update token!";
            return;
        }
    } else {
        if (!m_ioDB->insertToken(WickrIOTokens::getRandomString(TOKEN_LENGTH), cUser->id, "*")) {
            qDebug() << "Cannot add new token!";
            return;
        }
    }
    sendAuthEmail(cUser);
}

void
CmdHandler::sendAuthEmail(WickrIOConsoleUser *consoleUser)
{
    if (consoleUser->email.isEmpty() || m_email.server.isEmpty()) {
        return;
    }

    SmtpClient::ConnectionType type;
    if (m_email.type.toLower() == "smtp")
        type = SmtpClient::TcpConnection;
    else if (m_email.type.toLower() == "ssl")
        type = SmtpClient::SslConnection;
    else if (m_email.type.toLower() == "tls")
        type = SmtpClient::TlsConnection;

    SmtpClient *smtpClient = new SmtpClient(m_email.server, m_email.port, type);
    if (smtpClient == NULL) {
    } else {
        smtpClient->setUser(m_email.account);
        smtpClient->setPassword(m_email.password);
        smtpClient->connectToHost();
        smtpClient->login();

        MimeMessage message;
        message.setSender(new EmailAddress("test1@wickr.com"));
        message.addRecipient(new EmailAddress(consoleUser->email));
        message.setSubject("Authorization for WickrIO");

        MimeText text;
        QString msgStr = QString("Authorization should use key: %1").arg(consoleUser->token);
        text.setText(msgStr);
        message.addPart(&text);

        if (!smtpClient->sendMail(message)) {
            qDebug() << "Mail send failed!";
        }
        delete smtpClient;
    }
}

/**
 * @brief CmdHandler::updateSSLSettings
 * This function will update the values in the SSL Settings object. This should
 * be done for each request, just in case changes were made by the user.
 */
void
CmdHandler::updateSSLSettings()
{
    m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
    m_sslSettings.readFromSettings(m_settings);
    m_settings->endGroup();
}

/**
 * @brief CmdHandler::addClient
 * This function will add a client using the input HTTP parameters. The new client will be
 * left in the paused state.  An explicit command will be needed to start the client.
 * @param request
 * @param response
 */
void
CmdHandler::addClient(WickrIOConsoleUser *pCUser, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    // Need to verify that this user can add a new client
    if (pCUser->maxclients > 0) {
        int curCnt = m_ioDB->numConsoleClients(pCUser->id);
        if (curCnt >= pCUser->maxclients) {
            sendFailure(409, "Max clients limit reached", response);
            return;
        }
    }

    // Get the new client information from the URL
    QByteArray paramName = request.getParameter(APIPARAM_NAME);
    QByteArray paramUser = request.getParameter(APIPARAM_USER);
    QByteArray paramPassword = request.getParameter(APIPARAM_PASSWORD);
    QByteArray paramAPIKey = request.getParameter(APIPARAM_APIKEY);
    QByteArray paramInterface = request.getParameter(APIPARAM_IFACE);
    QByteArray paramPort = request.getParameter(APIPARAM_PORT);
    QByteArray paramIfaceType = request.getParameter(APIPARAM_IFACETYPE);
    QByteArray paramBinary = request.getParameter(APIPARAM_BOTTYPE);

    // Make sure the appropriate parameters exist
    if (paramName.length() == 0 || paramUser.length() == 0 || paramPassword.length() == 0 ||
        paramAPIKey.length() == 0 || paramInterface.length() == 0 || paramPort.length() == 0 ||
        paramBinary.length() == 0) {
        sendFailure(400, "Missing parameters", response);
        return;
    }

    WickrBotClients client;
    client.name = QString(paramName);
    client.user = QString(paramUser);
    client.password = QString(paramPassword);
    client.apiKey = QString(paramAPIKey);
    client.binary = QString(paramBinary);
    client.iface = QString(paramInterface);
    client.port = paramPort.toInt();
    if (!paramIfaceType.isEmpty() || QString(paramIfaceType).toLower() == APIPARAM_IFACET_HTTPS) {
        client.isHttps = true;
        updateSSLSettings();
        client.sslKeyFile = m_sslSettings.sslKeyFile;
        client.sslCertFile = m_sslSettings.sslCertFile;
    } else {
        client.isHttps = false;
    }
    client.console_id = pCUser->id;

    // Check that the Binary identifies a values binary type
    if (!WBIOServerCommon::isValidClientApp(client.binary)) {
        sendFailure(400, "Invalid client binary", response);
        return;
    }

    // Get all of the clients so that we can verify names and such are not duplicated
    {
        QList<WickrBotClients *> clients;
        clients = m_ioDB->getClients();
        bool failed = false;

        if (chkClientsNameExists(clients, client.name)) {
            sendFailure(400, "Name already used", response);
            failed = true;
//        } else if (chkClientsUserExists(clients, client.user)) {
//            sendFailure(400, "User already exists", response);
//            failed = true;
        } else  if (chkClientsInterfaceExists(clients, client.iface, client.port)) {
            sendFailure(400, "Interface port combination already in use", response);
            failed = true;
        }

        // free the memory associated with the clients records, not needed now
        for (WickrBotClients *client : clients) {
            delete client;
        }

        if (failed)
            return;
    }

    if (client.isHttps) {
        if (client.sslKeyFile.isEmpty() || client.sslCertFile.isEmpty()) {
            sendFailure(400, "SSL not configured!", response);
            return;
        }
    }

    // Add the new record to the database
    QString errorStr = WickrIOConsoleClientHandler::addClient(m_ioDB, &client);
    if (errorStr.isEmpty()) {
        qDebug() << "Successfully added record to the database!";
        getClient(pCUser, client.user, response);
    } else {
        qDebug() << "Failed to add record to the database:" << errorStr;
        sendFailure(409, errorStr.toLatin1(), response);
        return;
    }
}


/**
 * @brief CmdHandler::deleteClient
 * This function will handle the delete client api request.  The input client name
 * field is used to find the appropriate Client to delete from the database. The
 * client needs to be in the paused or stopped state for this action to work.
 * @param clientID
 * @param response
 */
void
CmdHandler::deleteClient(WickrIOConsoleUser *pCUser, const QString& clientID, stefanfrings::HttpResponse& response)
{
    WickrBotClients *client = m_ioDB->getClientUsingName(clientID);

    // If the client does not exist then write appropriate response
    if (client == NULL) {
        sendFailure(404, "Cannot find the specified client record", response);
        return;
    }

    // If the Console User is NOT and admin and the clients console ID is not
    // equal to the console user's id then fail!
    if (! pCUser->isAdmin() && pCUser->id != client->console_id) {
        sendFailure(403, "You do not have permission to delete that client", response);
    } else {
        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getClientProcessName(client);
        if (m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                sendFailure(409, "Client is currently running. Client must be in paused state!", response);
            } else if (! m_ioDB->deleteProcessState(processName)) {
                qDebug() << "There was a problem deleting the client's process record!";
            }
        }

        if (! m_ioDB->deleteClientUsingName(client->name)) {
            sendFailure(500, "Failed to delete client", response);
        } else {
            sendSuccess(response);
        }
    }

    // Cleanup any memory that was allocated
    delete client;

    // TODO: Need to cleanup the clients directory and registry (for windows)
}

/**
 * @brief CmdHandler::getClients
 * Get all of the clients currently known
 * @param request
 * @param response
 */
void
CmdHandler::getClients(WickrIOConsoleUser *pCUser, stefanfrings::HttpResponse& response)
{
#if 0
    QByteArray paramStart = request.getParameter(APIPARAM_START);
    QByteArray paramCount = request.getParameter(APIPARAM_COUNT);

    // Validate the API Key
    if (paramStart.length() == 0 || paramCount.length() == 0) {
        sendFailure(400, "Missing parameters", response);
        return;
    }
    int start = paramStart.toInt();
    int count = paramCount.toInt();
#endif

    QJsonArray clientArrayValue;
    QList<WickrBotClients *> clients;

    // Get the clients records.  If admin console user get all of the clients
    if (pCUser->isAdmin()) {
        clients = m_ioDB->getClients();
    } else {
        clients = m_ioDB->getConsoleClients(pCUser->id);
    }

    while (clients.length() > 0) {
        WickrBotClients * client = clients.first();
        clients.removeFirst();

        QJsonObject clientValue;

        clientValue.insert(APIJSON_NAME, client->name);
        clientValue.insert(APIJSON_APIKEY, client->apiKey);
        clientValue.insert(APIJSON_USER, client->user);
        clientValue.insert(APIJSON_IFACE, client->iface);
        clientValue.insert(APIJSON_PORT, client->port);
        clientValue.insert(APIJSON_IFACETYPE, client->getIfaceTypeStr());
        clientValue.insert(APIJSON_BOTTYPE, client->binary);
        int actionCnt = m_operation->m_botDB->getClientsActionCount(client->id);
        clientValue.insert(APIJSON_PENDINGMSGS, actionCnt);

        // Get the process state for this client
        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getClientProcessName(client);

        if (m_operation->m_botDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                clientValue.insert(APIJSON_STATE, APIJSON_STATE_RUNNING);
            } else if (state.state == PROCSTATE_DOWN) {
                clientValue.insert(APIJSON_STATE, APIJSON_STATE_DOWN);
            } else if (state.state == PROCSTATE_PAUSED) {
                clientValue.insert(APIJSON_STATE, APIJSON_STATE_PAUSED);
            } else {
                clientValue.insert(APIJSON_STATE, APIJSON_STATE_UNKNOWN);
            }
        } else {
            clientValue.insert(APIJSON_STATE, APIJSON_STATE_DOWN);
        }

        clientArrayValue.append(clientValue);

        delete client;
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_CLIENTS, clientArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

/**
 * @brief CmdHandler::getClient
 * This function will retrieve a specific client record.
 * @param clientID
 * @param response
 */
void
CmdHandler::getClient(WickrIOConsoleUser *pCUser, const QString& clientID, stefanfrings::HttpResponse& response)
{
    WickrBotClients * client = m_ioDB->getClientUsingName(clientID);

    // If the client does not exist then write appropriate response
    if (client == NULL) {
        sendFailure(409, "Client not found!", response);
        return;
    }

    // If the console user is not an admin and the console id is not the same then return failed
    if (!pCUser->isAdmin() && client->console_id != pCUser->id) {
        sendFailure(409, "Client not found!", response);
        delete client;
        return;
    }

    QJsonObject clientValue;

    clientValue.insert(APIJSON_NAME, client->name);
    clientValue.insert(APIJSON_APIKEY, client->apiKey);
    clientValue.insert(APIJSON_USER, client->user);
    clientValue.insert(APIJSON_IFACE, client->iface);
    clientValue.insert(APIJSON_PORT, client->port);
    clientValue.insert(APIJSON_BOTTYPE, client->binary);

    // Get the process state for this client
    WickrBotProcessState state;
    QString processName = WBIOServerCommon::getClientProcessName(client);

    if (m_ioDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_RUNNING) {
            clientValue.insert(APIJSON_STATE, APIJSON_STATE_RUNNING);
        } else if (state.state == PROCSTATE_DOWN) {
            clientValue.insert(APIJSON_STATE, APIJSON_STATE_DOWN);
        } else if (state.state == PROCSTATE_PAUSED) {
            clientValue.insert(APIJSON_STATE, APIJSON_STATE_PAUSED);
        } else {
            clientValue.insert(APIJSON_STATE, APIJSON_STATE_UNKNOWN);
        }
    } else {
        clientValue.insert(APIJSON_STATE, APIJSON_STATE_DOWN);
    }

    delete client;

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_CLIENT, clientValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

/**
 * @brief CmdHandler::updateClient
 * This command will implement the update client capability.  Currently the only value
 * that can be updated it the state of the client. The user can set the client's state
 * to pause or start.
 * @param clientID
 * @param request
 * @param response
 */
void
CmdHandler::updateClient(WickrIOConsoleUser *pCUser, const QString& clientID, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray paramState = request.getParameter(APIPARAM_STATE);
    int state;
    bool success=false;

    // Validate the state value to change
    if (paramState.length() == 0) {
        sendFailure(400, "Missing state value in request!", response);
        return;
    }

    // User can only change the state to either pause or start
    if (paramState.toLower() == APIPARAM_PAUSE) {
        // Pause the client
        state = PROCSTATE_PAUSED;
    } else if (paramState.toLower() == APIPARAM_START) {
        // Start the client
        state = PROCSTATE_RUNNING;
    } else if (paramState.toLower() == APIPARAM_RESET) {
        // Reset the client
        state = PROCSTATE_RESET;
    } else {
        sendFailure(400, "Invalid state value", response);
        return;
    }

    // Get the client record
    WickrBotClients * client = m_ioDB->getClientUsingName(clientID);

    // If the client does not exist then write appropriate response
    if (client == NULL) {
        sendFailure(409, "Unknown client", response);
    } else if (!pCUser->isAdmin() && client->console_id != pCUser->id) {
        // If the console user is not an admin and the console id is not the same then return failed
        sendFailure(409, "Client not found!", response);
    } else {
        // Perform the specified action
        if (state == PROCSTATE_PAUSED) {
            if (pauseClient(client)) {
                success = true;
            } else {
                sendFailure(409, "Failed to pause client", response);
            }
        } else if (state == PROCSTATE_RUNNING) {
            if (startClient(client)) {
                success = true;
            } else {
                sendFailure(409, "Failed to start client", response);
            }
        } else if (state == PROCSTATE_RESET) {
            bool doNextStep = true;

            // Get the process state for this client
            WickrBotProcessState state;
            QString processName = WBIOServerCommon::getClientProcessName(client);
            int savedState;

            if (m_ioDB->getProcessState(processName, &state)) {
                savedState = state.state;
                if (savedState == PROCSTATE_RUNNING) {
                    // Client is starting, so it should be paused first
                    // For now do not support reset of running client
                    sendFailure(409, "Client is running, stop the client first!", response);
                    doNextStep = false;
                } else if (savedState == PROCSTATE_DOWN) {
                    // The client state is DOWN, this is abnormal, usuall means it is starting up
                    // So we should pause the client, assume it is not running, then reset and start it up
                    // Set the state of the client process to paused
                    if (! m_ioDB->updateProcessState(processName, 0, PROCSTATE_PAUSED)) {
                        sendFailure(409, "Could not set Client state to paused!", response);
                        doNextStep = false;
                    }
                } else if (savedState == PROCSTATE_PAUSED) {
                    // Client is already paused
                } else {
                    sendFailure(409, "Client in unknown state!", response);
                    doNextStep = false;
                }

            } else {
                sendFailure(409, "Cannot get Client state!", response);
                doNextStep = false;
            }

            // Time to reset, which means delete the client's database files
            if (doNextStep) {
                QString clientDbDir = QString("%1/clients/%2/client").arg(m_operation->databaseDir).arg(client->name);
                QDir clientDir(clientDbDir);
                QStringList fileList = clientDir.entryList(QDir::Files);

                QStringList excludeList;
                excludeList.append("settings");
                excludeList.append("bootstrap");
                excludeList.append("ds.wic");

                foreach (QString s, excludeList) {
                    int index = fileList.indexOf(s);
                    if (index > -1)
                        fileList.takeAt(index);
                }

                foreach (const QString& fileName, fileList)
                {
                    qDebug().noquote().nospace() << "Deleting " << fileName;
#if 1
                    QString fullFilePathname = clientDbDir + "/" + fileName;
                    QFile *f = new QFile(fullFilePathname);
                    if (!f->remove()) {
                        qDebug().noquote().nospace() << "Failed to delete " << fullFilePathname;
                    }
                    f->deleteLater();
#endif
                }

                // Put the client to a specific state based on where we started
                if (savedState == PROCSTATE_DOWN) {
                }
            }
        }
    }

    delete client;

    if (success) {
        sendSuccess(response);
    }
}






/**
 * @brief CmdHandler::sendClientCmd
 * This function will send and wait for a successful sent of a message to a
 * client, with the input port number.
 * @param port
 * @param cmd
 * @return
 */
bool CmdHandler::sendClientCmd(const QString& name, const QString& cmd)
{
    bool retval = false;

    m_clientMsgInProcess = true;
    m_clientMsgSuccess = false;

    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();

    if (ipcSvc != nullptr && ipcSvc->sendMessage(name, true, cmd)) {
        QTimer timer;
        QEventLoop loop;

        loop.connect(ipcSvc, SIGNAL(signalMessageSent()), SLOT(quit()));
        loop.connect(ipcSvc, SIGNAL(signalMessageSendFailure()), SLOT(quit()));
        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

        int loopCount = 6;

        while (loopCount-- > 0) {
            timer.start(10000);
            loop.exec();

            if (timer.isActive()) {
                timer.stop();
                break;
            } else {
                qDebug() << "Timed out waiting for stop client message to send!";
            }
        }
        retval = true;
    }

    return retval;
}

/**
 * @brief CmdHandler::chkClientsNameExists
 * Check if the input name is already used by one of the client records
 * @param name
 * @return
 */
bool CmdHandler::chkClientsNameExists(const QList<WickrBotClients *> &clients, const QString& name)
{
    for (WickrBotClients *client : clients) {
        if (client->name == name) {
            qDebug() << "The input name is NOT unique!";
            return true;
        }
    }
    return false;
}

/**
 * @brief CmdHandler::chkClientsUserExists
 * Check if the input user is already used by one of the client records
 * @param name
 * @return
 */
bool CmdHandler::chkClientsUserExists(const QList<WickrBotClients *> &clients,const QString& user)
{
#if 0
    for (WickrBotClients *client : clients) {
        if (client->user == user) {
            qDebug() << "The input user name is NOT unique!";
            return true;
        }
    }
    return false;
#else
    return true;
#endif
}

/**
 * @brief CmdHandler::chkClientsInterfaceExists
 * Check if the input interface and port combination is already used
 * by one of the client records
 * @param name
 * @return
 */
bool CmdHandler::chkClientsInterfaceExists(const QList<WickrBotClients *> &clients,const QString& iface, int port)
{
    for (WickrBotClients *client : clients) {
        if (client->iface == iface && client->port == port) {
            qDebug() << "The input interface and port are NOT unique!";
            return true;
        }
    }
    return false;
}

/**
 * @brief CmdHandler::addClientData
 * This function will add the appropriate files and directories and then the specific
 * database entry for the new client. The client will be left in the paused state.
 * @param newClient
 * @return
 */
bool
CmdHandler::addClientData(WickrBotClients *newClient)
{
    // Create the configuration file
    QString dbDir(m_operation->databaseDir);
    QString logname;
    QString configFileName;
    QString clientDbDir;
    QString attachDir;

#ifdef Q_OS_WIN
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT)
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET)
            .arg(newClient->name);
#else
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(m_operation->databaseDir).arg(newClient->name);
#endif
    clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(m_operation->databaseDir).arg(newClient->name);
    logname = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(m_operation->databaseDir).arg(newClient->name);
    attachDir = QString(WBIO_CLIENT_ATTACHDIR_FORMAT).arg(m_operation->databaseDir).arg(newClient->name);

    // The client DB directory MUST exist already
    if (! QDir(m_operation->databaseDir).exists()) {
        qDebug() << m_operation->databaseDir << "does not exist";
        return false;
    }

    QString clientDir;

    clientDir = QString("%1/clients").arg(m_operation->databaseDir);
    if (! QDir(clientDir).exists()) {
        qDebug() << clientDir << "does not exist";
        if (! QDir().mkdir(clientDir)) {
            qDebug() << tr("Cannot create the %1 directory").arg(clientDir);
            return false;
        }
    }

    clientDir = QString("%1/clients/%2").arg(m_operation->databaseDir).arg(newClient->name);
    qDebug() << "ClientDir=" << clientDir;
    if (! QDir(clientDir).exists()) {
        qDebug() << clientDir << "does not exist";
        if (! QDir().mkdir(clientDir)) {
            qDebug() << tr("Cannot create the %1 directory").arg(clientDir);
            return false;
        }
    }

#ifdef Q_OS_LINUX
    QFile file(configFileName);
    if (file.exists()) {
        qDebug() << "Configuration file already exists, removing!";
        file.remove();
    }
#endif

    QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

    settings->beginGroup(WBSETTINGS_USER_HEADER);
    settings->setValue(WBSETTINGS_USER_USER, newClient->user);
    settings->setValue(WBSETTINGS_USER_PASSWORD, newClient->password);
    settings->setValue(WBSETTINGS_USER_USERNAME, newClient->name);
    settings->setValue(WBSETTINGS_USER_TRANSACTIONID, newClient->transactionID);
    settings->setValue(WBSETTINGS_USER_AUTOLOGIN, newClient->m_autologin);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    settings->setValue(WBSETTINGS_DATABASE_DIRNAME, dbDir);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    settings->setValue(WBSETTINGS_LOGGING_FILENAME, logname);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_LISTENER_HEADER);
    settings->setValue(WBSETTINGS_LISTENER_PORT, newClient->port);
    if (newClient->iface == "localhost") {
        settings->remove(WBSETTINGS_LISTENER_IF);
    } else {
        settings->setValue(WBSETTINGS_LISTENER_IF, newClient->iface);
    }
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_ATTACH_HEADER);
    settings->setValue(WBSETTINGS_ATTACH_DIRNAME, attachDir);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_SERVICES_HEADER);
    settings->setValue(WBSETTINGS_SERVICES_HANDLEINBOX, newClient->m_handleInbox);
    settings->endGroup();

    settings->sync();

    // Insert a client record into the database
    if (m_operation->m_botDB->insertClientsRecord(newClient)) {
        QString processName = WBIOServerCommon::getClientProcessName(newClient);

        // Set the state of the client process to paused
        if (! m_operation->m_botDB->updateProcessState(processName, 0, PROCSTATE_PAUSED))
            qDebug() << "updateProcessState returned false";
        return true;
    } else {
        return false;
    }
}

/**
 * @brief CmdHandler::pauseClient
 * Pause the input client. Sends a pause message to the client's IPC port, if
 * defined. Any error will return a false value.
 * @param client
 * @return
 */
bool
CmdHandler::pauseClient(WickrBotClients *client)
{
    WickrBotProcessState state;
    QString processName = WBIOServerCommon::getClientProcessName(client);

    if (m_operation->m_botDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_RUNNING) {
            if (sendClientCmd(client->name, WBIO_IPCCMDS_PAUSE)) {
                return true;
            }
            qDebug() << "Failed to send message to client!";
        } else if (state.state == PROCSTATE_PAUSED) {
            return true;
        } else {
            qDebug() << "Client must be running to pause it!";
        }
    } else {
        qDebug() << "Could not get the clients state!";
    }
    return false;
}

/**
 * @brief CmdHandler::startClient
 * Start the input client.
 * @param client
 * @return
 */
bool
CmdHandler::startClient(WickrBotClients *client)
{
    WickrBotProcessState state;
    QString processName = WBIOServerCommon::getClientProcessName(client);

    if (m_operation->m_botDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_PAUSED) {
            if (m_operation->m_botDB->updateProcessState(processName, 0, PROCSTATE_DOWN)) {
                return true;
            }
            qDebug() << "Failed to change start of client in database!";
        } else if (state.state == PROCSTATE_DOWN){
            qDebug() << "Client is in Down state. The WickrIO Client Server should change the state to running.";
            qDebug() << "If this is not happening, please check that the WickrIOSvr process is running!";
            //TODO: Check on the state of the WickrIO Server!
        } else if (state.state == PROCSTATE_RUNNING) {
            return true;
        }
    } else {
        qDebug() << "Could not get the clients state!";
    }
    return false;
}







/******* USERS *******/

void
CmdHandler::addUser(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    // Get the new user information from the URL
    QByteArray paramUser = request.getParameter(APIPARAM_USER);
    QByteArray paramPW = request.getParameter(APIPARAM_PASSWORD);
    QByteArray paramUserType = request.getParameter(APIPARAM_USERTYPE);
    QByteArray paramMaxClients = request.getParameter(APIPARAM_MAXCLIENTS);
    QByteArray paramAuthType = request.getParameter(APIPARAM_AUTHTYPE);
    QByteArray paramEmail = request.getParameter(APIPARAM_EMAIL);

    // Make sure the appropriate parameters exist
    if (paramUser.length() == 0 || paramPW.length() == 0 || paramUserType.length() == 0 || paramMaxClients.length() == 0 ||
        paramAuthType.isEmpty()) {
        sendFailure(400, "Missing parameters", response);
        return;
    }

    WickrIOConsoleUser cUser;
    if (QString(paramAuthType).toLower() == APIPARAM_ATYPE_BASIC) {
        cUser.authType = CUSER_AUTHTYPE_BASIC;
    } else if (QString(paramAuthType).toLower() == APIPARAM_ATYPE_EMAIL) {
        if (paramEmail.isEmpty()) {
            sendFailure(400, "Authentication type email requres email addres", response);
            return;
        }
        cUser.authType = CUSER_AUTHTYPE_EMAIL;
    } else {
        sendFailure(400, "Invalid authentication type", response);
        return;
    }

    cUser.user = QString(paramUser);
    cUser.password = QString(paramPW);
    cUser.maxclients = paramMaxClients.toInt();
    cUser.setAdmin(paramUserType.toLower() == APIPARAM_UTYPE_ADMIN);
    cUser.email = QString(paramEmail);

    // Get all of the clients so that we can verify names and such are not duplicated
    WickrIOConsoleUser testCUser;
    if (m_ioDB->getConsoleUser(cUser.user, &testCUser)) {
        sendFailure(400, "User name already used", response);
        return;
    }

    // Add the new record to the database
    if (m_ioDB->insertConsoleUser(&cUser)) {
        qDebug() << "Successfully added record to the database!";
        getUser(cUser.user, response);
    } else {
        qDebug() << "Failed to add record to the database";
        sendFailure(409, "Database error!", response);
        return;
    }
}


void
CmdHandler::deleteUser(const QString& clientID, stefanfrings::HttpResponse& response)
{
    WickrIOConsoleUser cUser;

    // If the client does not exist then write appropriate response
    if (! m_ioDB->getConsoleUser(clientID, &cUser)) {
        sendFailure(404, "Cannot find the specified user record", response);
        return;
    }

    if (! m_ioDB->deleteConsoleUser(cUser.id)) {
        sendFailure(500, "Failed to delete user", response);
    } else {
        sendSuccess(response);
    }
}

void
CmdHandler::getUsers(stefanfrings::HttpResponse& response)
{
    QJsonArray userArrayValue;
    QList<WickrIOConsoleUser *> cusers;
    cusers = m_ioDB->getConsoleUsers();

    while (cusers.length() > 0) {
        WickrIOConsoleUser * cuser = cusers.first();
        cusers.removeFirst();

        QJsonObject userValue;

        userValue.insert(APIJSON_USER, cuser->user);
        userValue.insert(APIJSON_MAXCLIENTS, cuser->maxclients);
        userValue.insert(APIJSON_USERTYPE, cuser->isAdmin() ? APIJSON_UTYPE_ADMIN : APIJSON_UTYPE_USER);
        userValue.insert(APIJSON_CANEDIT, cuser->canEdit());
        userValue.insert(APIJSON_CANCREATE, cuser->canCreate());
        userValue.insert(APIJSON_RXEVENTS, cuser->rxEvents());
        userValue.insert(APIJSON_AUTHTYPE, cuser->getAuthTypeStr());
        userValue.insert(APIJSON_EMAIL, cuser->email);

        userArrayValue.append(userValue);

        delete cuser;
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_USERS, userArrayValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

void
CmdHandler::getUser(const QString& clientID, stefanfrings::HttpResponse& response)
{
    WickrIOConsoleUser cuser;

    // If the user does not exist then write appropriate response
    if (!m_ioDB->getConsoleUser(clientID, &cuser)) {
        sendFailure(409, "User not found!", response);
        return;
    }

    QJsonObject userValue;
    userValue.insert(APIJSON_USER, cuser.user);
    userValue.insert(APIJSON_MAXCLIENTS, cuser.maxclients);
    userValue.insert(APIJSON_USERTYPE, cuser.isAdmin() ? APIJSON_UTYPE_ADMIN : APIJSON_UTYPE_USER);
    userValue.insert(APIJSON_CANEDIT, cuser.canEdit());
    userValue.insert(APIJSON_CANCREATE, cuser.canCreate());
    userValue.insert(APIJSON_RXEVENTS, cuser.rxEvents());
    userValue.insert(APIJSON_AUTHTYPE, cuser.getAuthTypeStr());
    userValue.insert(APIJSON_EMAIL, cuser.email);

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_USER, userValue);
    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

void
CmdHandler::updateUser(const QString& clientID, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QByteArray paramMaxClients = request.getParameter(APIPARAM_MAXCLIENTS);
    bool success=false;

    // Validate the state value to change
    if (paramMaxClients.length() == 0) {
        sendFailure(400, "Missing value in request!", response);
        return;
    }

    // Get the client record
    WickrIOConsoleUser cuser;

    if (!m_ioDB->getConsoleUser(clientID, &cuser)) {
        sendFailure(409, "Unknown user", response);
    } else {
    }

    if (success) {
        sendSuccess(response);
    }
}

QJsonObject
CmdHandler::getClientStats(WickrBotClients *client)
{
    QJsonObject statValues;
    statValues.insert(APIJSON_STATID_PENDING, m_operation->m_botDB->getClientsActionCount(client->id));

    QList<WickrBotStatistics *> stats;
    stats = m_operation->m_botDB->getClientStatistics(client->id);
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
    return statValues;
}

void
CmdHandler::getStatistics(stefanfrings::HttpResponse& response)
{
    QJsonArray clientArrayValue;

    QList<WickrBotClients *>clients = m_operation->m_botDB->getClients();

    for (WickrBotClients *client : clients) {
        QJsonObject clientValues;

        clientValues.insert(APIJSON_NAME, client->name);

        clientValues.insert(APIJSON_STATISTICS, getClientStats(client));
        clientArrayValue.append(clientValues);
    }

    QJsonObject jsonObject;
    jsonObject.insert(APIJSON_CLIENTS, clientArrayValue);

    QJsonDocument saveDoc(jsonObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

void
CmdHandler::getStatistics(const QString& clientName, stefanfrings::HttpResponse& response)
{
    QJsonObject statsObject;
    WickrBotClients *client = m_operation->m_botDB->getClientUsingName(clientName);
    if (client != NULL) {
        statsObject.insert(APIJSON_STATISTICS, getClientStats(client));
    }
    QJsonDocument saveDoc(statsObject);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}
