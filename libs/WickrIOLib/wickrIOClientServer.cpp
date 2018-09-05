#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "wickrIOClientServer.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrIOIPCRuntime.h"

//#define DEBUG_TRACE 1

/**
 * @brief WickrIOClientServer::WickrIOClientServer
 * This is the constructor for the WickrIO service (windows) / Daemon (linux)
 */
WickrIOClientServer::WickrIOClientServer(QObject *parent) : QObject(parent),
    m_vendorName(QString(WBIO_ORGANIZATION))
{
}

WickrIOClientServer::~WickrIOClientServer()
{
}

/**
 * @brief WickrIOClientServer::configureService
 * This function will configure the setup of this service.  Specifically the
 * database file location, log file location, output file location will be
 * setup.
 * @return True is returned if the service is configured, false if not
 */
bool WickrIOClientServer::configureService()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering configureService";
#endif

    QSettings* settings = WBIOServerCommon::getSettings();
    m_operation->databaseDir = WBIOServerCommon::getDBLocation();

    QString logFileName("");

    // If there is a database directory then create the Wickrbot database
    m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);


    /*
     * Setup the logs file
     */
    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    logFileName = settings->value(WBSETTINGS_LOGGING_FILENAME).toString();
    settings->endGroup();

    // If a logfile name is NOT set then put a log file into the database directory
    if (logFileName.isEmpty()) {
        logFileName = m_operation->databaseDir + "/logs/" + m_operation->processName + ".log";
    }

    // Check that can create the log file
    QFileInfo fileInfo(logFileName);
    QDir dir;
    dir = fileInfo.dir();

    if (!WBIOCommon::makeDirectory(dir.path())) {
        qDebug() << "WickrBot Server cannot make log directory:" << dir;
    } else {
        m_operation->log_handler->setupLog(logFileName);
        m_operation->log_handler->log("WickrIO Client Server configured");
    }

    m_operation->log_handler->log("Database size", m_operation->m_botDB->size());
    m_operation->log_handler->log("Generated messages", m_operation->messageCount);

#if 0
    // Setup the IPC Service
    m_ipcSvc = new WickrIOIPCService(WBIO_CLIENTSERVER_TARGET, false);
    m_ipcSvc->startIPC(m_operation);
    connect(m_ipcSvc, &WickrIOIPCService::signalReceivedMessage, this, &WickrIOClientServer::slotRxIPCMessage);
#else
    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();
    connect(ipcSvc, &WickrIOIPCService::signalReceivedMessage, this, &WickrIOClientServer::slotRxIPCMessage);
#endif

    m_isConfigured = true;

#ifdef DEBUG_TRACE
    qDebug() << "Leaving configureService";
#endif
    return true;
}

/**
 * @brief WickrIOClientServer::processStarted
 * Called when the thread is started. This will start a timer that
 * will call the slotTimeoutProcess() slot on one second intervals, to perform the
 * tasks associated with this service.
 */
void WickrIOClientServer::processStarted(bool resume)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering start";
#endif
    if (resume) {
        getClients(true);
    } else {
        m_statusCntDwn = UPDATE_STATUS_SECS;
        m_backOff = BACK_OFF_START;
        m_backOffCntDwn = m_backOff;

        if (m_processTimer != NULL) {
            m_processTimer->deleteLater();
            m_processTimer = NULL;
        }
    }

    m_processTimer = new QTimer(0);
    connect(m_processTimer, &QTimer::timeout, this, &WickrIOClientServer::slotTimeoutProcess);
    m_processTimer->setInterval(1000);
    m_processTimer->start();
#ifdef DEBUG_TRACE
    qDebug() << "Leaving start";
#endif
}

/**
 * @brief WickrIOClientServer::processFinished
 * Called when the thread is finished. This directive will send
 * the WBIO_IPCCMDS_STOP IPC command to all of the active clients.
 */
void WickrIOClientServer::processFinished(bool pause)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering stop";
#endif
    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    getClients(false);

    if (!pause) {
        m_operation->updateProcessState(PROCSTATE_DOWN);
        QCoreApplication::quit();
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving stop";
#endif
}


/**
 * @brief slotGotMessage
 * @param type
 * @param value
 */
void
WickrIOClientServer::slotRxIPCMessage(QString type, QString value)
{
    if (type.toLower() == WBIO_IPCMSGS_USERSIGNKEY) {
        qDebug().noquote() << "Received User Signing Key:" << value;
    } else if (type.toLower() == WBIO_IPCMSGS_STATE) {
        qDebug().noquote() << "Client changed state to" << value;
        m_clientStateChanged = true;
        m_clientState = value;
    } else if (type.toLower() == WBIO_IPCMSGS_BOTINFO) {
        qDebug().noquote() << "Received BOT Information message:" << value;
        QMap<QString,QString> valMap;
        valMap = WickrIOIPCCommands::parseBotInfoValue(value);
        QString processValue = valMap.value(WBIO_BOTINFO_PROCESSNAME);
        QString passwordValue = valMap.value(WBIO_BOTINFO_PASSWORD);

        qDebug().noquote().nospace() << "BotInfo:";
        qDebug().noquote().nospace() << "    Sender: " << valMap.value(WBIO_IPCHDR_PROCESSNAME);
        qDebug().noquote().nospace() << "    Client: " << valMap.value(WBIO_BOTINFO_CLIENT);
        qDebug().noquote().nospace() << "    Process: " << processValue;

        // If the entry already exists for this client then remove it
        if (m_clientPasswords.contains(processValue)) {
            m_clientPasswords.remove(processValue);
        }
        // Add the process and password value for this client
        m_clientPasswords.insert(processValue, passwordValue);
    } else {
        qDebug().noquote() << "Received unhandled message=" << type << ", value=" << value;
    }
}


/**
 * @brief WickrIOClientServer::slotTimeoutProcess
 * This function is called when the service timer expires every
 * second after service has been started.  When called this
 * function will only perform actions if the service has been configured. For
 * Windows this means the registry has been setup, for Linux this requires the
 * settings INI file to be setup. If everything is configured then the list of
 * client process' will be checked and if necessary the clients will be started.
 * If starting of the client processes fail, it will double the time waited between
 * starting the clients up to 1 minute
 */
void WickrIOClientServer::slotTimeoutProcess()
{
    if (m_isConfigured || configureService()) {
        if (--m_statusCntDwn < 0) {
            m_operation->updateProcessState(PROCSTATE_RUNNING);
            m_statusCntDwn = UPDATE_STATUS_SECS;
        }
    }

    if (--m_backOffCntDwn < 0){
        if (getClients(true) == true){
            m_backOff = BACK_OFF_START;
            m_backOffCntDwn = m_backOff;
        }
        else {
            if (m_backOff < BACK_OFF_MAX){
                m_backOff = m_backOff * 2;
            }
            m_backOffCntDwn = m_backOff;
        }
    }
}

/**
 * @brief WickrIOClientServer::clientNeedsStart
 * Checks if the client needs to be started. If the client is running already or is
 * in the paused state then it does not need to be started and this will return false.
 * If client stopped or running with old timestamp then will return true to indicated
 * client should be restarted
 * @param name the name of the client found in the process_state table
 * @return
 */
bool WickrIOClientServer::clientNeedsStart(WickrBotClients *client)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering clientNeedsStart";
#endif
    WickrBotProcessState procState;
    if (m_operation->m_botDB == NULL) {
#ifdef DEBUG_TRACE
        qDebug() << "Leaving clientNeedsStart: no DB!";
#endif
        return false;
    }

    // Get the process name to check against, in the process state table
    QString processName = WBIOServerCommon::getClientProcessName(client);
    if (m_operation->m_botDB->getProcessState(processName, &procState)) {
        /*
         * If the previous run is still in the running state then evaluate whether
         * that process is still running or not
         */
        if (procState.state == PROCSTATE_RUNNING) {
#ifdef DEBUG_TRACE
            m_operation->log_handler->log(QString("Process is still running, date=%1").arg(procState.last_update.toString(DB_DATETIME_FORMAT)));
#endif
            const QDateTime dt = QDateTime::currentDateTime();
            int secs = procState.last_update.secsTo(dt);
#ifdef DEBUG_TRACE
            m_operation->log_handler->log(QString("Seconds since last status:%1").arg(QString::number(secs)));
#endif
            // If less than timeout (3 minutes) then return failed, since the process is still running
            if (!m_operation->csStarted && !m_operation->force && secs < m_operation->m_appTimeOut) {
                // Return false to identify that it is already active and does not need to be started
#ifdef DEBUG_TRACE
                qDebug() << "Leaving clientNeedsStart: secs is < 120 or no force!";
#endif
                return false;
            }

            // Else it has been longer than timeout (3 minutes), kill the old process and continue
            if (WickrBotUtils::isRunning(client->binary, procState.process_id)) {
                m_operation->log_handler->log(QString("Killing old process, id=%1").arg(procState.process_id));
                WickrBotUtils::killProcess(procState.process_id);
                return true;
            }
        } else if (procState.state == PROCSTATE_PAUSED) {
#ifdef DEBUG_TRACE
            qDebug() << "Leaving clientNeedsStart: proc state is paused already!";
#endif
            return false;
        }
    }

    // Return true to identify that the process needs to be started
#ifdef DEBUG_TRACE
    qDebug() << "Leaving clientNeedsStart";
#endif
    return true;
}

bool
WickrIOClientServer::sendClientCmd(const QString& dest, const QString& cmd)
{
    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();

    qDebug() << "Sending command to client";
    if (ipcSvc == nullptr || !ipcSvc->sendMessage(dest, true, cmd)) {
        return false;
    }

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
    return true;
}


/**
 * @brief WickrIOClientServer::stopClient
 * This function will send a stop message to the input client.
 * @param client
 * @return
 */
bool WickrIOClientServer::stopClient(const QString& name)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entered stopClient:" << client->user;
#endif
    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();
    // Send message to all clients to stop them
    if (ipcSvc == nullptr || ! ipcSvc->sendMessage(name, true, WBIO_IPCCMDS_STOP))
        return false;

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

#ifdef DEBUG_TRACE
    qDebug() << "Leaving stopClient: retVal=" << retVal;
#endif
    return true;
}

/**
 * @brief startClient
 * This function will start a specific client application.
 * @param operation
 * @param client
 * @param state
 * @return
 */
bool WickrIOClientServer::startClient(WickrBotClients *client)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering startClient";
#endif
    QString dbDir(m_operation->databaseDir);
    QString logname;
    QString configFileName;
    QString clientDbDir;
    QString workingDir;
    QString processName = WBIOServerCommon::getClientProcessName(client);

    // Check if the client process is still active
    if (! clientNeedsStart(client)) {
#ifdef DEBUG_TRACE
        m_operation->log_handler->log(QString("Note: Process does not need to start for %1").arg(client->name));
        qDebug() << "Leaving startClient: proc not need to start!";
#endif
        return true;
    }
#ifdef Q_OS_WIN
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_ORGANIZATION).arg(WBIO_GENERAL_TARGET).arg(client->name);
    clientDbDir = QString("%1\\clients\\%2\\client").arg(m_operation->databaseDir).arg(client->name);
    logname = QString("%1\\clients\\%2\\logs\\WickrIO%2.log").arg(m_operation->databaseDir).arg(client->name);
    workingDir = QString("%1\\clients\\%2").arg(m_operation->databaseDir).arg(client->name);

    QString outputFile = QString("%1\\clients\\%2\\logs\\WickrIO%2.output").arg(m_operation->databaseDir).arg(client->name);
#else
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(m_operation->databaseDir).arg(client->name);
    clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(m_operation->databaseDir).arg(client->name);
    logname = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(m_operation->databaseDir).arg(client->name);
    workingDir = QString(WBIO_CLIENT_WORKINGDIR_FORMAT).arg(m_operation->databaseDir).arg(client->name);

    QString outputFile = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(m_operation->databaseDir).arg(client->name);
#endif

    QString outputDirName = QString(WBIO_CLIENT_LOGDIR_FORMAT).arg(m_operation->databaseDir).arg(client->name);
    QDir    outputDir(outputDirName);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputDirName)) {
            qDebug().noquote() << QString("CONSOLE:Cannot create log directory %1").arg(outputDirName);
            outputDirName = "";
        }
    }

    m_operation->log_handler->log("**********************************");
    m_operation->log_handler->log(QString("startClient: command line arguments for %1").arg(client->name));
    m_operation->log_handler->log(QString("dbDir=%1").arg(dbDir));
    m_operation->log_handler->log(QString("logname=%1").arg(logname));
    m_operation->log_handler->log(QString("Output File=%1").arg(outputFile));
    m_operation->log_handler->log(QString("Config File=%1").arg(configFileName));
    m_operation->log_handler->log(QString("Client DB Dir=%1").arg(clientDbDir));
    m_operation->log_handler->log("**********************************");

#ifdef Q_OS_LINUX
    // Check for the existence of the Config file.  Cannot continue if it does not exist
    QFile configFile(configFileName);
    if (!configFile.exists()) {
        m_operation->log_handler->log(QString("Error: config file does not exist for %1").arg(client->name));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: config file not exist!";
#endif
        return false;
    }
#endif

    // Check that the User name is configured
    QSettings* settings = new QSettings(configFileName, QSettings::NativeFormat);
    settings->beginGroup(WBSETTINGS_USER_HEADER);
    QString user = settings->value(WBSETTINGS_USER_USER, "").toString();
    if (user.isEmpty()) {
        m_operation->log_handler->log(QString("Error: config registry does not contain User name for %1").arg(client->name));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: registry not contain user name!";
#endif
        return false;
    }
    settings->endGroup();

    // Add an output file name so that output can be saved
    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
    if (curOutputFilename.isEmpty()) {
        settings->setValue(WBSETTINGS_LOGGING_OUTPUT_FILENAME, outputFile);
        settings->endGroup();
        settings->sync();
    } else {
        settings->endGroup();
    }

    // Done with the clients settings
    settings->deleteLater();


    // Check that we have the needed information to proceed
    bool needsPassword = WBIOServerCommon::isPasswordRequired(client->binary);

    // If the password is needed then prompt for it
    if (!needsPassword) {
        if (client->m_autologin) {
            // Check if the database password has been created.
            // If not then will need the client's password to start.
            QString clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
            QString dbKeyFileName = QString("%1/dkd.wic").arg(clientDbDir);
            QFile dbKeyFile(dbKeyFileName);
            if (!dbKeyFile.exists()) {
                needsPassword = true;
            }
        } else {
            needsPassword = true;
        }
    }

    if (needsPassword && !m_clientPasswords.contains(processName)) {
        qDebug() << "Leaving startClient: need password for" << client->name;
        return false;
    }

    // Clear the DB entry for the client: state and PID
    m_operation->m_botDB->updateProcessState(processName, 0, PROCSTATE_DOWN);

    // Start the client application for the specific client/user
    QStringList arguments;
    QString command;

    command = client->binary;

#if 1
    arguments.append(QString("-clientname=%1").arg(client->user));
#else
    arguments.append(QString("-config=%1").arg(configFileName));
    arguments.append(QString("-clientdbdir=%1").arg(clientDbDir));
    arguments.append(QString("-processname=%1").arg(processName));
#endif

    QProcess exec;

    // If any errors occur put them in the output file
    connect(&exec, &QProcess::errorOccurred, [=](QProcess::ProcessError error)
    {
        qDebug() << "error enum val = " << error;
    });



#ifdef QT5.9.4
    if (exec.startDetached(command, arguments, workingDir)) {
#else
    QProcess process;
    process.setProgram(command);
    process.setArguments(arguments);

    if (!outputDirName.isEmpty()) {
        process.setStandardOutputFile(outputFile);
        process.setStandardErrorFile(outputFile);
    }
    qint64 pid;
    if (process.startDetached(&pid)) {
#endif

        m_operation->log_handler->log(QString("Started client for %1").arg(client->name));

        // For clients that require a password, send it over
        if (needsPassword) {
            qDebug() << "Client needs password!";

            WickrBotProcessState state;

            while ( true ) {
                if (m_operation->m_botDB->getProcessState(processName, &state)) {
                    // Can't get the process state, what to do
                }
                if (state.state != PROCSTATE_RUNNING) {
                    qDebug().noquote() << QString("Waiting for %1 to start").arg(client->name);
                    QThread::sleep(1);
                    continue;
                }

                // It is running lets send the password to it now
                QString password = m_clientPasswords.value(processName);
                QString pwstring = WickrIOIPCCommands::getPasswordString(WBIO_CLIENTSERVER_TARGET, password);
                sendClientCmd(client->name, pwstring);

                // Need to check that the password worked
                while (true) {
                    QCoreApplication::processEvents();
                    if (m_clientStateChanged) {
                        if (m_clientState == "loggedin") {
                            qDebug().noquote() << QString("CONSOLE:%1 is logged in").arg(client->name);
                        } else if (m_clientState == "stopping") {
                            qDebug().noquote() << QString("CONSOLE:%1 to login!").arg(client->name);
                        }
                        break;
                    }
                }
                break;
            }

        }
    } else {
        m_operation->log_handler->log(QString("Could NOT start client for %1").arg(client->name));
        m_operation->log_handler->log(QString("command=%1").arg(command));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: could not start!";
#endif
        return false;
    }

    // Check if there is an integration to start for this client
    if (!client->botType.isEmpty()) {
        QString startCmd = WBIOServerCommon::getBotStartCmd(client->botType);
        if (!startCmd.isEmpty()) {
            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(client->botType);
            QString startFullPath = QString("%1/%2").arg(destPath).arg(startCmd);

            m_operation->log_handler->log("**********************************");
            m_operation->log_handler->log(QString("Starting %1").arg(startFullPath));

            // Create the argument list for the start script, the client's Wickr ID
            QStringList arguments;
            arguments.append(client->user);

#ifdef QT5.9.4
            if (!QProcess::startDetached(startFullPath, arguments, destPath)) {
                qDebug() << QString("Failed to run %1").arg(startFullPath);
            } else {
            }
#else
            QProcess process;
            process.setProgram(startFullPath);
            process.setArguments(arguments);

            process.setWorkingDirectory(destPath);
            QString outputfile = QString(WBIO_INTEGRATION_OUTFILE_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name);

            process.setStandardOutputFile(outputfile);
            process.setStandardErrorFile(outputfile);
            qint64 pid;
            process.startDetached(&pid);
#endif

            m_operation->log_handler->log("Done starting!");
            m_operation->log_handler->log("**********************************");
        }
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving startClient";
#endif
    return true;
}

/**
 * @brief WickrIOClientServer::getClients
 * This function will get a list of clients and then either start or stop them. If unable to start one will return false,
 * otherwise will return true
 */
bool WickrIOClientServer::getClients(bool start)
{
    bool allClientsStart = true;
    bool checkForParser = false;
    QList<WickrBotClients *> clients;
    QString parserExecutable;
    QString parserName;
    QList <WickrBotProcessState*> processes;

    clients = m_operation->m_botDB->getClients();

    if (clients.size() > 0) {
//        m_operation->log_handler->log("List of clients:");
        foreach (WickrBotClients *client, clients) {
            WickrBotProcessState state;
            QString processName = WBIOServerCommon::getClientProcessName(client);

            if (m_operation->m_botDB->getProcessState(processName, &state)) {
//                m_operation->log_handler->log(QString("process=%1, id=%2, process=%3, state=%4").arg(state.process).arg(state.id).arg(state.process_id).arg(state.state));
            }

            if (start) {
                if(startClient(client) == false)
                    allClientsStart = false;
            } else {
                stopClient(client->name);
            }
            //check whether type is welcome_bot to see if need to find parser
            if(client->binary.startsWith( "welcome_bot")){
                checkForParser = true;
            }
        }
        if( checkForParser == true){
            processes = m_operation->m_botDB->getProcessStates();
            for(WickrBotProcessState* process :processes)
            {
                if(process->process.startsWith("WelcomeBotParser")){
                    parserExecutable=WBIOServerCommon::getParserApp();
                    parserName = process->process;
                    if(start){
                        if(parserNeedsStart(process)){
                            m_operation->log_handler->log(QString("process=%1, id=%2, state=%3").arg(process->process).arg(process->id).arg(process->state));
                            if(startParser(parserName,parserExecutable)== false)
                                allClientsStart= false;
                        }
                    }
                    else{
                        //If start is false, shutdown parsers
                        stopClient(parserName);
                    }
                    process->deleteLater();
                }
                else{
                    process->deleteLater();
                }
            }
        }
    }

    // Turn off the Client Server started flag, if was set.
    if (m_operation->csStarted)
        m_operation->csStarted = false;

    return allClientsStart;
}



/**
 * @brief WickrIOClientServer::startParser
 * Starts Parser with QProcess
 */
bool WickrIOClientServer::startParser(QString processName, QString appName)
{
    QString configFileName;
    QString outputFile;


#ifdef Q_OS_WIN
//need to find the values for config File and output file on windows
    configFileName="";
    outputFile="";
#else
    configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT).arg(m_operation->m_botDB->m_dbDir).arg(processName);
    outputFile = QString(WBIO_PARSER_LOGFILE_FORMAT).arg(m_operation->m_botDB->m_dbDir).arg(processName);

#endif

    m_operation->log_handler->log("**********************************");
    m_operation->log_handler->log(QString("startParser: command line arguments for %1").arg(processName));
    m_operation->log_handler->log(QString("Config File=%1").arg(configFileName));
    m_operation->log_handler->log(QString("Executable name: %1").arg(appName));
    m_operation->log_handler->log("**********************************");

    QFile configFile(configFileName);
    if(!configFile.exists()){
        m_operation->log_handler->log(QString("Error: config file does not exist for %1").arg(processName));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startParser: config file does not exist";
#endif
        return false;
    }

    QStringList arguments;
    arguments.append(QString("-appName=%1").arg(processName));

#ifdef QT5.9.4
    if (QProcess::startDetached(appName, arguments)) {
        m_operation->log_handler->log(QString("Started parser %1").arg(processName));
        return true;
    }
    else {
        m_operation->log_handler->log(QString("Could NOT start client for %1").arg(processName));
        return false;
    }
#else
    QProcess process;
    process.setProgram(appName);
    process.setArguments(arguments);

    process.setStandardOutputFile(outputFile);
    process.setStandardErrorFile(outputFile);
    qint64 pid;
    process.startDetached(&pid);
#endif

}

/**
 * @brief WickrIOClientServer::parserNeedsStart
 * Checks if the parser needs to be started. If it is running already with recent
 * last update or is in the paused state then it does not need to be started and
 * this will return false. If it is paused with old timestamp, will kill process
 * and return true, as the process should be restarted.
 * @param name the name of the client found in the process_state table
 * @return
 */
bool WickrIOClientServer::parserNeedsStart(WickrBotProcessState *process)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering parserNeedsStart";
#endif
    if (m_operation->m_botDB == NULL) {
#ifdef DEBUG_TRACE
        qDebug() << "Leaving parserNeedsStart: no DB!";
#endif
        return false;
    }
    //If state is running, check that it has not silently failed
    if (process->state == PROCSTATE_RUNNING) {
#ifdef DEBUG_TRACE
        m_operation->log_handler->log(QString("Process is still running, date=%1").arg(process.last_update.toString(DB_DATETIME_FORMAT)));
#endif
        const QDateTime dt = QDateTime::currentDateTime();
        int secs = process->last_update.secsTo(dt);
#ifdef DEBUG_TRACE
            m_operation->log_handler->log(QString("Seconds since last status:%1").arg(QString::number(secs)));
#endif
            // If less than 3 minutes then return failed, since the process is still running
            if (!m_operation->force && secs < m_operation->m_appTimeOut) {
                // Return false to identify that it is already active and does not need to be restarted
#ifdef DEBUG_TRACE
                qDebug() << "Leaving parserNeedsStart: Has been less than 3 minutes and not forcing!";
#endif
                return false;
            }
    QString binary = process->process;
    // Else it has been longer than timeout (3 minutes), kill the old process and continue
    //binary not be correct. Could need to use executable name
    if (WickrBotUtils::isRunning(binary, process->process_id)) {
        m_operation->log_handler->log(QString("Killing old process, id=%1").arg(process->process_id));
        WickrBotUtils::killProcess(process->process_id);
        return true;
    }
       } else if (process->state == PROCSTATE_PAUSED) {
#ifdef DEBUG_TRACE
            qDebug() << "Leaving parserNeedsStart: proc state is paused already!";
#endif
            return false;
        }

#ifdef DEBUG_TRACE
    qDebug() << "Leaving parserNeedsStart";
#endif
    return true;
}
