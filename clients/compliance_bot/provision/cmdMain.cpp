#include <QTextStream>
#include <QDebug>

#include "cmdMain.h"
#include "wickrIOBot.h"
#include "wbio_common.h"
#include "clientconfigurationinfo.h"

CmdMain::CmdMain(QCoreApplication *app, int argc, char **argv) :
    m_app(app),
    m_argc(argc),
    m_argv(argv),
    m_ipc(this)
{
}

CmdMain::~CmdMain()
{
    if (m_ioDB->isOpen()) {
        m_ioDB->close();
    }

    for (WickrIOClients *client : m_clients) {
        delete client;
    }
    m_clients.clear();
}

bool
CmdMain::runCommands()
{
    // TODO: Need to add an entry into the client record table
    m_ioDB = new WickrIOClientDatabase(WBIO_DEFAULT_DBLOCATION);
    if (!m_ioDB->isOpen()) {
        qDebug() << "CONSOLE:cannot open database!";
        return false;
    }

    updateBotList();
    QTextStream input(stdin);

    while (true) {
        qDebug() << "CONSOLE:Enter command [list, add, start, stop]:";
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            int clientIndex;

            // Convert the second argument to an integer, for the client index commands
            if (args.size() > 1) {
                bool ok;
                clientIndex = args.at(1).toInt(&ok);
                if (!ok) {
                    qDebug() << "CONSOLE:Client Index is not a number!";
                    continue;
                }
            } else {
                clientIndex = -1;
            }

            if (cmd == "list") {
                listBots();
#if 0
            } else if (cmd == "advanced") {
                if (!m_cmdAdvanced.runCommands())
                    break;
#endif
            } else if (cmd == "add") {
                // FOR NOW lets add a new user
                WickrIOBot *newbot = new WickrIOBot(m_app, m_argc, m_argv);
                if (newbot->newBotCreate()) {

                } else {

                }
            } else if (cmd == "pause") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: pause <index>";
                } else {
                    pauseClient(clientIndex);
                }
            } else if (cmd == "start") {
                if (clientIndex == -1) {
                    qDebug() << "CONSOLE:Usage: start <index>";
                } else {
                    startClient(clientIndex);
                }
            } else if (cmd == "quit") {
                qDebug() << "CONSOLE:Good bye!";
                break;
            } else if (cmd == "?") {
                qDebug() << "CONSOLE:Enter one of:";
                qDebug() << "CONSOLE:  list   - to setup the clients";
                qDebug() << "CONSOLE:  add    - to setup the advanced settings";
                qDebug() << "CONSOLE:  quit     - to exit the program";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
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
            if (m_ioDB->getProcessState(processName, &state)) {
                if (state.state == PROCSTATE_DOWN) {
                    clientState = "Down";
                } else if (state.state == PROCSTATE_RUNNING) {
                    clientState = "Running";
                } else if (state.state == PROCSTATE_PAUSED) {
                    clientState = "Paused";
                }
            }
            qDebug().noquote() << QString("CONSOLE:client[%1]").arg(++i);
            qDebug().noquote() << QString("CONSOLE:  name=%1").arg(client->name);
            qDebug().noquote() << QString("CONSOLE:  user=%1").arg(client->user);
            qDebug().noquote() << QString("CONSOLE:  state=%1").arg(clientState);
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
CmdMain::startClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrIOClients *client = m_clients.at(clientIndex);
        WickrBotProcessState state;
        QString processName = client->getProcessName();

        if (m_ioDB->getProcessState(processName, &state)) {
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
        } else {
            qDebug() << "CONSOLE:Could not get the clients state!";
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

    if (! m_ipc.sendMessage(port, cmd)) {
        return false;
    }

    QTimer timer;
    QEventLoop loop;

    loop.connect(&m_ipc, SIGNAL(signalSentMessage()), SLOT(quit()));
    loop.connect(&m_ipc, SIGNAL(signalSendError()), SLOT(quit()));
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

