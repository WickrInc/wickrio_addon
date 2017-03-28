#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "wickrioclientserverservice.h"
#include "wbio_common.h"
#include "server_common.h"

extern bool isVERSIONDEBUG();

#define DEBUG_TRACE 1

/**
 * @brief WickrIOClientServerService::WickrIOClientServerService
 * This is the constructor for the WickrIO service (windows) / Daemon (linux)
 * @param argc
 * @param argv
 */
WickrIOClientServerService::WickrIOClientServerService(int argc, char **argv) :
    QtService<QCoreApplication>(argc, argv, WBIO_CLIENTSERVER_TARGET),
    m_operation(NULL),
    m_processTimer(NULL),
    m_vendorName(QString(WBIO_ORGANIZATION)),
    m_isConfigured(false)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering constructor";
#endif

    setServiceDescription(QObject::tr("WickrIO Client Server Service"));
    setServiceFlags(QtServiceBase::CanBeSuspended);

    m_appNm = WBIO_CLIENTSERVER_TARGET;

    m_operation = new OperationData();
    m_operation->processName = WBIO_CLIENTSERVER_TARGET;

    if (configureService()) {
        m_operation->updateProcessState(PROCSTATE_RUNNING);
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving constructor";
#endif
}

WickrIOClientServerService::~WickrIOClientServerService()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering destructor";
#endif
    m_operation->updateProcessState(PROCSTATE_DOWN);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving destructor";
#endif
}

/**
 * @brief WickrIOClientServerService::configureService
 * This function will configure the setup of this service.  Specifically the
 * database file location, log file location, output file location will be
 * setup.
 * @return True is returned if the service is configured, false if not
 */
bool WickrIOClientServerService::configureService()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering configureService";
#endif

    QSettings* settings = WBIOServerCommon::getSettings();
    m_operation->databaseDir = WBIOServerCommon::getDBLocation();

    QString logFileName("");

    // If there is a database directory then create the Wickrbot database
    m_operation->m_botDB = new WickrBotDatabase(m_operation->databaseDir);


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
        m_operation->setupLog(logFileName);
        m_operation->log("WickrIO Client Server configured");
    }

    m_operation->log("Database size", m_operation->m_botDB->size());
    m_operation->log("Generated messages", m_operation->messageCount);

    m_isConfigured = true;

#ifdef DEBUG_TRACE
    qDebug() << "Leaving configureService";
#endif
    return true;
}

/**
 * @brief WickrIOClientServerService::slotTimeoutProcess
 * This function is called when the service timer expires, which is every
 * second. This is only when the service has been started.  When called this
 * function will only perform actions if the service has been configured. For
 * Windows this means the registry has been setup, for Linux this requires the
 * settings INI file to be setup. If everything is configured then the list of
 * client process' will be checked and if necessary the clients will be started.
 */
void WickrIOClientServerService::slotTimeoutProcess()
{
    if (m_isConfigured || configureService()) {
        if (--m_statusCntDwn < 0) {
            m_operation->updateProcessState(PROCSTATE_RUNNING);
            m_statusCntDwn = UPDATE_STATUS_SECS;
        }

        getClients(true);
    }
}

/**
 * @brief WickrIOClientServerService::start
 * This is a service directive to start the service. This will start a timer that
 * will call the slotTimeoutProcess() slot on one second intervals, to perform the
 * tasks associated with this service.
 */
void WickrIOClientServerService::start()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering start";
#endif
    m_statusCntDwn = UPDATE_STATUS_SECS;

    if (m_processTimer != NULL) {
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    m_processTimer = new QTimer(0);
    connect(m_processTimer, &QTimer::timeout, this, &WickrIOClientServerService::slotTimeoutProcess);
    m_processTimer->setInterval(1000);
    m_processTimer->start();
#ifdef DEBUG_TRACE
    qDebug() << "Leaving start";
#endif
}

/**
 * @brief WickrIOClientServerService::stop
 * This is the stop service directive to stop this service. This directive will send
 * the WBIO_IPCCMDS_STOP IPC command to all of the active clients.
 */
void WickrIOClientServerService::stop()
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

    m_operation->updateProcessState(PROCSTATE_DOWN);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving stop";
#endif
}

/**
 * @brief WickrIOClientServerService::pause
 * This is the pause service directive to pause this service. A WBIO_IPCCMDS_STOP IPC
 * command will be sent to all of the clients.
 */
void WickrIOClientServerService::pause()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering pause";
#endif
    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    getClients(false);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving pause";
#endif
}

/**
 * @brief WickrIOClientServerService::resume
 * This is the resume service directive to resume this service. The clients
 * will be restarted.
 */
void WickrIOClientServerService::resume()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering resume";
#endif
    // Start all of the clients
    getClients(true);

    m_processTimer = new QTimer(0);
    QObject::connect(m_processTimer, &QTimer::timeout, this, &WickrIOClientServerService::slotTimeoutProcess);
    m_processTimer->setInterval(1000);
    m_processTimer->start();
#ifdef DEBUG_TRACE
    qDebug() << "Leaving resume";
#endif
}

/**
 * @brief WickrIOClientServerService::processCommand
 * @param code
 */
void WickrIOClientServerService::processCommand(int code)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering processCommand";
#endif
    QString cmd = QString("received processCommand = %1").arg(code);
    qDebug() << "in processCommand:" << cmd;
#ifdef DEBUG_TRACE
    qDebug() << "Leaving processCommand";
#endif
}

/**
 * @brief WickrIOClientServerService::clientNeedsStart
 * Checks if the client needs to be started. If the client is running already or is
 * in the paused state then it does not need to be started.
 * @param name the name of the client found in the process_state table
 * @return
 */
bool WickrIOClientServerService::clientNeedsStart(WickrBotClients *client)
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
            m_operation->log(QString("Process is still running, date=%1").arg(procState.last_update.toString(DB_DATETIME_FORMAT)));

            const QDateTime dt = QDateTime::currentDateTime();
            int secs = procState.last_update.secsTo(dt);
            m_operation->log(QString("Seconds since last status:%1").arg(QString::number(secs)));

            // If less than 2 minutes then return failed, since the process is still running
            if (!m_operation->force && secs < 120) {
                // Return true to identify that it is already active
#ifdef DEBUG_TRACE
                qDebug() << "Leaving clientNeedsStart: secs is < 120 or no force!";
#endif
                return false;
            }

            // Else it has been longer than 10 minutes, kill the old process and continue
            if (WickrBotUtils::isRunning(client->binary, procState.process_id)) {
                m_operation->log(QString("Killing old process, id=%1").arg(procState.process_id));
                WickrBotUtils::killProcess(procState.process_id);
            }
        } else if (procState.state == PROCSTATE_PAUSED) {
#ifdef DEBUG_TRACE
            qDebug() << "Leaving clientNeedsStart: proc state is paused already!";
#endif
            return false;
        }
    }

    // Return false to identify that the process is not running already
#ifdef DEBUG_TRACE
    qDebug() << "Leaving clientNeedsStart";
#endif
    return true;
}

/**
 * @brief WickrIOClientServerService::stopClient
 * This function will send a stop message to the input client.
 * @param client
 * @return
 */
bool WickrIOClientServerService::stopClient(const WickrBotProcessState& state)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entered stopClient:" << state.process << ", ipc_port=" << state.ipc_port;
#endif
    // Send message to all clients to stop them
    bool retVal = true;

    if (state.ipc_port != 0) {
        // Setup the IPC Client to communicate with the clients
        WickrBotIPC ipc;

        // TODO: Need to wait to see if the message returns successfully!!!
        retVal = ipc.sendMessage(state.ipc_port, WBIO_IPCCMDS_STOP);

        QTimer timer;
        QEventLoop loop;

        loop.connect(&ipc, SIGNAL(signalSentMessage()), SLOT(quit()));
        loop.connect(&ipc, SIGNAL(signalSendError()), SLOT(quit()));
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
    }

#ifdef DEBUG_TRACE
    qDebug() << "Leaving stopClient: retVal=" << retVal;
#endif
    return retVal;
}

/**
 * @brief startClient
 * This function will start a specific client application.
 * @param operation
 * @param client
 * @param state
 * @return
 */
bool WickrIOClientServerService::startClient(WickrBotClients *client)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering startClient";
#endif
    QString dbDir(m_operation->databaseDir);
    QString logname;
    QString configFileName;
    QString clientDbDir;
    QString workingDir;

    // Check if the client process is still active
    if (! clientNeedsStart(client)) {
        m_operation->log(QString("Note: Process does not need to start for %1").arg(client->name));
#ifdef DEBUG_TRACE
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
    clientDbDir = QString("%1/clients/%2/client").arg(m_operation->databaseDir).arg(client->name);
    logname = QString("%1/clients/%2/logs/WickrIO%2.log").arg(m_operation->databaseDir).arg(client->name);
    workingDir = QString("%1/clients/%2").arg(m_operation->databaseDir).arg(client->name);

    QString outputFile = QString("%1/clients/%2/logs/WickrIO%2.output").arg(m_operation->databaseDir).arg(client->name);
#endif


    m_operation->log("**********************************");
    m_operation->log(QString("startClient: command line arguments for %1").arg(client->name));
    m_operation->log(QString("dbDir=%1").arg(dbDir));
    m_operation->log(QString("logname=%1").arg(logname));
    m_operation->log(QString("Output File=%1").arg(outputFile));
    m_operation->log(QString("Config File=%1").arg(configFileName));
    m_operation->log(QString("Client DB Dir=%1").arg(clientDbDir));
    m_operation->log("**********************************");

#ifdef Q_OS_LINUX
    // Check for the existence of the Config file.  Cannot continue if it does not exist
    QFile configFile(configFileName);
    if (!configFile.exists()) {
        m_operation->log(QString("Error: config file does not exist for %1").arg(client->name));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: config file not exist!";
#endif
        return false;
    }
#endif

    // Check that the User name is configured
    QSettings* settings=new QSettings(configFileName, QSettings::NativeFormat);
    settings->beginGroup(WBSETTINGS_USER_HEADER);
    QString user = settings->value(WBSETTINGS_USER_USER, "").toString();
    if (user.isEmpty()) {
        m_operation->log(QString("Error: config registry does not contain User name for %1").arg(client->name));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: registry not contain user name!";
#endif
        return false;
    }
    settings->endGroup();

    // Add an output file name so that output can be saved
    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
    if (curOutputFilename.isEmpty())
        settings->setValue(WBSETTINGS_LOGGING_OUTPUT_FILENAME, outputFile);
    settings->endGroup();

    // Done with the clients settings
    settings->deleteLater();


    // Start the client application for the specific client/user
    QStringList arguments;
    QString command;

    command = client->binary;

    arguments.append(QString("-config=%1").arg(configFileName));
    arguments.append(QString("-clientdbdir=%1").arg(clientDbDir));
    arguments.append(QString("-processname=%1").arg(WBIOServerCommon::getClientProcessName(client)));

    QProcess exec;
    exec.setStandardOutputFile(outputFile);
    exec.setProcessChannelMode(QProcess::MergedChannels);
    if (exec.startDetached(command, arguments, workingDir)) {
        m_operation->log(QString("Started client for %1").arg(client->name));
    } else {
        m_operation->log(QString("Could NOT start client for %1").arg(client->name));
        m_operation->log(QString("command=%1").arg(command));
#ifdef DEBUG_TRACE
        qDebug() << "Leaving startClient: could not start!";
#endif
        return false;
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving startClient";
#endif
    return true;
}

/**
 * @brief WickrIOClientServerService::getClients
 * This function will get a list of clients and then either start or stop them
 */
void WickrIOClientServerService::getClients(bool start)
{
    QList<WickrBotClients *> clients;

    clients = m_operation->m_botDB->getClients();

    if (clients.size() > 0) {
//        m_operation->log("List of clients:");
        foreach (WickrBotClients *client, clients) {
            WickrBotProcessState state;
            QString processName = WBIOServerCommon::getClientProcessName(client);

            if (m_operation->m_botDB->getProcessState(processName, &state)) {
//                m_operation->log(QString("process=%1, id=%2, process=%3, state=%4").arg(state.process).arg(state.id).arg(state.process_id).arg(state.state));
            }

            if (start) {
                startClient(client);
            } else {
                stopClient(state);
            }
        }
    }
}

