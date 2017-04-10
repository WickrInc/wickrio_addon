#include <QTextStream>
#include <QDebug>

#include "cmdMain.h"
#include "wickrIOBot.h"
#include "wbio_common.h"
#include "wickrbotsettings.h"
#include "clientconfigurationinfo.h"
#include "wickrbotutils.h"

CmdMain::CmdMain(QCoreApplication *app, int argc, char **argv, OperationData *operation) :
    m_app(app),
    m_argc(argc),
    m_argv(argv),
    m_txIPC(this),
    m_operation(operation)
{
}

CmdMain::~CmdMain()
{
    for (WickrIOClients *client : m_clients) {
        delete client;
    }
    m_clients.clear();
}

/**
 * @brief slotGotMessage
 * @param type
 * @param value
 */
void CmdMain::signalReceivedMessage(QString type, QString value)
{
    if (type.toLower() == WBIO_IPCMSGS_USERSIGNKEY) {
        qDebug() << "CONSOLE:********************************************************************";
        qDebug() << "CONSOLE:**** USER SIGNING KEY";
        qDebug() << "CONSOLE:**** You will need this to enter into the console for the Bot";
        qDebug() << "CONSOLE:****";
        qDebug() << "CONSOLE:" << value;
        qDebug() << "CONSOLE:********************************************************************";
    } else if (type.toLower() == WBIO_IPCMSGS_STATE) {
        qDebug() << "Client changed state to" << value;
        m_clientStateChanged = true;
        m_clientState = value;
    }
}

bool
CmdMain::runCommands()
{
    // Set a pointer to the WickrIO Database, in the parent class
    m_ioDB = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);

    QTextStream input(stdin);

    while (true) {
        updateBotList();
        qDebug() << "CONSOLE:Enter command [list, add, config, start, stop]:";
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            int clientIndex;
            bool bForce = false;

            // Convert the second argument to an integer, for the client index commands
            if (args.size() > 1) {
                bool ok;
                clientIndex = args.at(1).toInt(&ok);
                if (!ok) {
                    qDebug() << "CONSOLE:Client Index is not a number!";
                    continue;
                }

                if (args.size() == 3) {
                    if (args.at(2) == "-force") {
                        bForce = true;
                    }
                }
            } else {
                clientIndex = -1;
            }

            if (cmd == "list") {
                listBots();
            } else if (cmd == "config") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: config <index>";
                } else {
                    configClient(clientIndex);
                }
#if 0
            } else if (cmd == "advanced") {
                if (!m_cmdAdvanced.runCommands())
                    break;
#endif
            } else if (cmd == "add") {
                // FOR NOW lets add a new user
                WickrIOBot *newbot = new WickrIOBot(m_app, m_argc, m_argv, m_ioDB);
                newbot->newBotCreate();
            } else if (cmd == "delete") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: delete <index>";
                } else {
                    deleteClient(clientIndex);
                }
            } else if (cmd == "reset") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: reset <index>";
                } else {
                    resetClient(clientIndex);
                }
            } else if (cmd == "stop") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: stop <index>";
                } else {
                    pauseClient(clientIndex);
                }
            } else if (cmd == "start") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: start <index>";
                } else {
                    startClient(clientIndex, bForce);
                }
            } else if (cmd == "quit") {
                qDebug() << "CONSOLE:Good bye!";
                break;
            } else if (cmd == "?") {
                qDebug() << "CONSOLE:Enter one of:";
                qDebug() << "CONSOLE:  list           - show list of clients";
                qDebug() << "CONSOLE:  add            - add a new client";
                qDebug() << "CONSOLE:  config <index> - configure a client";
                qDebug() << "CONSOLE:  delete <index> - delete a client";
                qDebug() << "CONSOLE:  reset <index>  - reset a client";
                qDebug() << "CONSOLE:  start <index>  - start a specific client";
                qDebug() << "CONSOLE:  stop <index>   - pause a running client";
                qDebug() << "CONSOLE:  quit           - exit the program";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }

    m_rxIPC->deleteLater();
    return true;
}

bool
CmdMain::updateBotList()
{
    for (WickrIOClients *client : m_clients) {
        delete client;
    }
    m_clients.clear();

    // TODO: Need to add an entry into the client record table
    QList<WickrIOClients *> clients = m_ioDB->getClients();

    for (WickrIOClients *client : clients) {
        if (client->binary != WBIO_BOT_TARGET) {
            delete client;
            continue;
        }
        m_clients.append(client);
    }
    return true;
}

bool
CmdMain::listBots()
{
    int i=0;
    if (m_clients.size() > 0) {
        for (WickrIOClients *client : m_clients) {
            if (client->binary != WBIO_BOT_TARGET)
                continue;

            // send the stop command to the client
            WickrBotProcessState state;
            QString clientState = "UNKNOWN";
            QString processName = client->getProcessName();
            QString procRunningString;
            if (m_ioDB->getProcessState(processName, &state)) {
                if (state.state == PROCSTATE_DOWN) {
                    clientState = "Down";
                } else if (state.state == PROCSTATE_RUNNING) {
                    clientState = "Running";
                    // Check if the process is running
                    if (!WickrBotUtils::isRunning(client->binary, state.process_id)) {
                        procRunningString = "(appears to be stopped)";
                    }
                } else if (state.state == PROCSTATE_PAUSED) {
                    clientState = "Paused";
                }
            }

            qDebug().noquote() << QString("CONSOLE:client[%1]").arg(i++);
            qDebug().noquote() << QString("CONSOLE:  name=%1").arg(client->name);
            qDebug().noquote() << QString("CONSOLE:  user=%1").arg(client->user);
            qDebug().noquote() << QString("CONSOLE:  state=%1 %2").arg(clientState).arg(procRunningString);
        }
    }
    if (i==0) {
        qDebug() << "CONSOLE:There are currently no clients configured";
    }
    return true;
}

/**
 * @brief CmdMain::validateIndex
 * Validate the input index. If invalid output a message
 * @param clientIndex
 */
bool CmdMain::validateIndex(int clientIndex)
{
    if (clientIndex >= m_clients.length() || clientIndex < 0) {
        qDebug() << "CONSOLE:The input client index is out of range!";
        return false;
    }
    return true;
}


/**
 * @brief CmdClient::startClient
 * This function is used to start a stopped or paused client
 * @param clientIndex
 */
void
CmdMain::startClient(int clientIndex, bool force)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);
        WickrBotProcessState state;
        QString processName = client->getProcessName();

        if (m_ioDB->getProcessState(processName, &state)) {
#if 0
            if (state.state == PROCSTATE_PAUSED) {
                QString prompt = QString(tr("Do you really want to start the client with the name %1")).arg(client->name);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    if (! m_ioDB->updateProcessState(processName, 0, PROCSTATE_DOWN)) {
                        qDebug() << "CONSOLE:Failed to change start of client in database!";
                    }
                }
            } else if (state.state == PROCSTATE_DOWN){
                qDebug() << "CONSOLE:Client is in Down state. The WickrIO Client Server should change the state to running.";
                qDebug() << "CONSOLE:If this is not happening, please check that the WickrIOSvr process is running!";
                //TODO: Check on the state of the WickrIO Server!
            } else {
                qDebug() << "CONSOLE:Client must be in paused state to start it!";
            }
#else
            if (!force && state.state == PROCSTATE_RUNNING) {
                // TODO: CHeck if the client is actually running
            } else {
                // Going to force a start
                // TODO: Check if the client is actually running


#ifdef Q_OS_WIN
                configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_ORGANIZATION).arg(WBIO_GENERAL_TARGET).arg(client->name);
                clientDbDir = QString("%1\\clients\\%2\\client").arg(m_operation->databaseDir).arg(client->name);
                logname = QString("%1\\clients\\%2\\logs\\WickrIO%2.log").arg(m_operation->databaseDir).arg(client->name);
                workingDir = QString("%1\\clients\\%2").arg(m_operation->databaseDir).arg(client->name);

                QString outputFile = QString("%1\\clients\\%2\\logs\\WickrIO%2.output").arg(m_operation->databaseDir).arg(client->name);
#else
                QString configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
                QString clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
                QString workingDir = QString(WBIO_CLIENT_WORKINGDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
                QString outputFile = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
#endif

                QSettings* settings=new QSettings(configFileName, QSettings::NativeFormat);

                // Add an output file name so that output can be saved
                settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
                QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
                if (curOutputFilename.isEmpty())
                    settings->setValue(WBSETTINGS_LOGGING_OUTPUT_FILENAME, outputFile);
                settings->endGroup();


                // Start the client application for the specific client/user
                QStringList arguments;
                QString command;

                command = client->binary;

                arguments.append(QString("-config=%1").arg(configFileName));
                arguments.append(QString("-clientdbdir=%1").arg(clientDbDir));
                arguments.append(QString("-processname=%1").arg(client->getProcessName()));
                if (force)
                    arguments.append("-force");

                m_clientStateChanged = false;

                QProcess exec;
                exec.setStandardOutputFile(outputFile);
                exec.setProcessChannelMode(QProcess::MergedChannels);
                if (exec.startDetached(command, arguments, workingDir)) {
                    qDebug().noquote() << QString("CONSOLE:Starting %1").arg(client->name);
                    QString prompt = QString("Enter password for %1").arg(client->name);
                    QString password = getNewValue("", prompt);

                    while ( true ) {
                        if (!m_ioDB->getProcessState(processName, &state)) {
                            // Can't get the process state, what to do
                        }
                        if (state.state != PROCSTATE_RUNNING) {
                            qDebug().noquote() << QString("CONSOLE:Waiting for %1 to start").arg(client->name);
                            QThread::sleep(1);
                            continue;
                        }

                        // It is running lets send the password to it now
                        QString pwstring = QString("%1=%2").arg(WBIO_IPCMSGS_PASSWORD).arg(password);
                        sendClientCmd(state.ipc_port, pwstring);

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
                } else {
                    qDebug().noquote() << QString("CONSOLE:Could NOT start %1").arg(client->name);
                    qDebug().noquote() << QString("CONSOLE:command=%1").arg(command);
            #ifdef DEBUG_TRACE
                    qDebug() << "Leaving startClient: could not start!";
            #endif
                }

            }
#endif
        } else {
            qDebug().noquote() << QString("CONSOLE:Could NOT get the state for %1").arg(client->name);
        }
    }
}

/**
 * @brief CmdClient::pauseClient
 * This function is used to pause a running client
 * @param clientIndex
 */
void
CmdMain::pauseClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);
        WickrBotProcessState state;
        QString processName = client->getProcessName();

        if (m_ioDB->getProcessState(processName, &state)) {
            if (state.ipc_port == 0) {
                qDebug() << "CONSOLE:Client does not have an IPC port defined, cannot pause!";
            } else if (state.state == PROCSTATE_RUNNING) {
                QString prompt = QString(tr("Do you really want to pause the client with the name %1")).arg(client->name);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    if (! sendClientCmd(state.ipc_port, WBIO_IPCCMDS_PAUSE)) {
                        qDebug() << "CONSOLE:Failed to send message to client!";
                    }
                }
            } else {
                qDebug() << "CONSOLE:Client must be running to pause it!";
            }
        } else {
            qDebug() << "CONSOLE:Could not get the clients state!";
        }
    }
}

/**
 * @brief CmdMain::sendClientCmd
 * This function will send and wait for a successful sent of a message to a
 * client, with the input port number.
 * @param port
 * @param cmd
 * @return
 */
bool
CmdMain::sendClientCmd(int port, const QString& cmd)
{
    m_clientMsgInProcess = true;
    m_clientMsgSuccess = false;

    if (! m_txIPC.sendMessage(port, cmd)) {
        return false;
    }

    QTimer timer;
    QEventLoop loop;

    loop.connect(&m_txIPC, SIGNAL(signalSentMessage()), SLOT(quit()));
    loop.connect(&m_txIPC, SIGNAL(signalSendError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 6;

    while (loopCount-- > 0) {
        timer.start(10000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "CONSOLE:Timed out waiting for stop client message to send!";
        }
    }
    return true;
}

/**
 * @brief CmdClient::startClient
 * This function is used to configure a specific client
 * @param clientIndex
 */
void
CmdMain::configClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);
        WickrIOAppSettings appSetting;
        bool bCallbackExists;

        appSetting.type = DB_APPSETTINGS_TYPE_MSGRECVCALLBACK;
        appSetting.id = 0;

        if (!m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
            appSetting.clientID = client->id;
            bCallbackExists = false;
            qDebug() << "CONSOLE:No message callback plugin setup";
        } else {
            bCallbackExists = true;
            qDebug().noquote() << QString("CONSOLE:Callback plugin: %1").arg(appSetting.value);
         }

        QString temp = getNewValue("y", tr("Client has callback plugin (y or n)?"));

        if (temp.toLower().startsWith("y")) {
            QString newCBack = getNewValue(appSetting.value, tr("Enter message callback plugin"));
            if (newCBack.toLower() == "back")
                return;

            appSetting.value = newCBack;

            if (m_ioDB->updateAppSetting(&appSetting)) {

            } else {

            }
        } else {
            // Double check that user wants to remove
            if (bCallbackExists) {
                QString temp = getNewValue("n", tr("Are you sure you want to remove the plugin (y or n)?"));
                if (temp.toLower().startsWith("y")) {
                    m_ioDB->deleteAppSetting(appSetting.id);
                    appSetting.value.clear();
                }
            }
        }

        qDebug().noquote() << QString("CONSOLE:Callback plugin: %1").arg(appSetting.value);
    }
}


/**
 * @brief CmdClient::deleteClient
 * This function is used to delete a client
 * this function will delete all of the files and database entries for the client
 * @param clientIndex
 */
void
CmdMain::deleteClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);

        QString prompt = QString(tr("Do you really want to delete the client with the name %1")).arg(client->name);
        QString response = getNewValue("", prompt);
        if (response.toLower() == "y" || response.toLower() == "yes") {
            QString processName = client->getProcessName();

            m_ioDB->deleteProcessState(processName);
            m_ioDB->deleteClientUsingName(client->name);

            // Remove the files associated with the client
            QString clientDirName = QString(WBIO_CLIENT_WORKINGDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
            QDir clientDir(clientDirName);
            if (clientDir.exists()) {
                clientDir.removeRecursively();
            }
        }
    }
}

/**
 * @brief CmdClient::resetClient
 * This function is used to reset a client.
 * This will remove the client database files for the client.
 * @param clientIndex
 */
void
CmdMain::resetClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);

        QString prompt = QString(tr("Do you really want to reset the client with the name %1")).arg(client->name);
        QString response = getNewValue("", prompt);
        if (response.toLower() == "y" || response.toLower() == "yes") {
            // Remove the files associated with the client
            QString clientDirName = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);

            QDir dir(clientDirName);
            QStringList name_filters;
            name_filters << "wickr_db.sqlite.*";
            name_filters << "*.wic";
            QFileInfoList fil = dir.entryInfoList(name_filters);
            for (QFileInfo finfo : fil) {
                if (finfo.isFile()) {
                    QFile file(finfo.filePath());
                    file.remove();
                }
            }
        }
    }
}

