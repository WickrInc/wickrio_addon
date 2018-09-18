#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QTemporaryFile>

#include "cmdclient.h"
#include "cmdserver.h"

#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"
#include "wickrIOIPCRuntime.h"
#include "wickrIOReturnCodes.h"

CmdClient::CmdClient(CmdOperation *operation) :
    m_operation(operation),
    m_cmdIntegration(operation)
{
    updateIntegrationVersion();
}

void
CmdClient::updateIntegrationVersion()
{
    QList<WBIOBotTypes *>integrations = WBIOServerCommon::getBotsSupported("wickrio_bot", false);
    m_integrationVersions.clear();

    for (WBIOBotTypes *integration : integrations) {
        QString versionName;
        if (integration->customBot()) {
            versionName = QString(WBIO_CUSTOMBOT_VERSIONFILE).arg(integration->name());
        } else {
            versionName = QString("%1/%2/VERSION").arg(WBIO_INTEGRATIONS_DIR).arg(integration->name());
        }

        QFile curHubotVersionFile(versionName);
        unsigned curHubotVersion;
        if (curHubotVersionFile.exists()) {
            curHubotVersion = getVersionNumber(&curHubotVersionFile);
            if (curHubotVersion > 0) {
                m_integrationVersions.insert(integration->name(), curHubotVersion);
            }
        }
    }
}

unsigned
CmdClient::getVersionNumber(QFile *versionFile)
{
    if (!versionFile->open(QIODevice::ReadOnly))
        return 0;

    QString s;

    QTextStream s1(versionFile);
    s.append(s1.readAll());
    versionFile->close();

    unsigned retVal = 0;
    QStringList slist = s.split(".");
    if (slist.length() == 3) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000 + slist.at(2).toInt();
    } else if (slist.length() == 2) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000;
    }
    return retVal;
}

void
CmdClient::getIntegrationVersionString(unsigned versionNum, QString& versionString)
{
    if (versionNum > 1000000) {
        versionString = QString("%1.%2.%3").arg(QString::number(versionNum / 1000000))
                                           .arg(QString::number((versionNum % 1000000) / 1000))
                                           .arg(QString::number(versionNum % 1000));
    } else if (versionNum > 1000) {
        versionString = QString("0.%1.%2").arg(QString::number(versionNum / 1000))
                                          .arg(QString::number(versionNum % 1000));
    } else if (versionNum == 0) {
        versionString = "unknown";
    } else {
        versionString = QString::number(versionNum);
    }
}

void
CmdClient::processHelp(const QStringList& cmdList)
{
    if (cmdList.length() > 0) {
        for (QString cmd : cmdList) {
            if (cmd == "add") {
                qDebug().nospace() << "CONSOLE:Add command usage: add\n"
                        << "  The add command is used to add a WickrIO bot client. You will be required to\n"
                        << "  enter specific information depending on the type of bot you are creating. The\n"
                        << "  minimum is the bots name and password. NOTE: password values are not shown or\n"
                        << "  saved on disk! You will be prompted for an option to use automatic logon for\n"
                        << "  the WickrIO bot client. This will require an initial password entry, but no\n"
                        << "  password will have to be entered after that. \n"
                        << "  For WickrIO bot clients that are associated with an integration you will have\n"
                        << "  to enter the name of the integration (custom or pre-packaged). You may also\n"
                        << "  have to enter other information related to that integration. Custom\n"
                        << "  integrations can be created via the integration command.\n";
            } else if (cmd == "back" && !m_root) {
                qDebug().nospace() << "CONSOLE:Back command usage: back\n"
                        << "  The back command will take you to the previous level of commands.\n";
            } else if (cmd == "config") {
                qDebug().nospace() << "CONSOLE:Config command usage: config\n"
                        << "  The config command is used to read in a conf file that contains details of\n"
                        << "  clients that will be added to this WickrIO system. This command can only be\n"
                        << "  used to create new WickrIO bots.\n"
                        << "  The format of the config file uses the ini file format, for example:\n\n"
                        << "    [clients]\n"
                        << "    my_new_bot=true\n\n"
                        << "    [my_new_bot]\n"
                        << "    auto_login=\"true\"\n\n"
                        << "  As shown above, the main group in the config file is identified by the\n"
                        << "  \"clients\" key. This group contains the list of client names which are used\n"
                        << "  to identify WickrIO bot clients to be added to the system. Each of these\n"
                        << "  client names are used as keys, to a group, that identify the specific bot\n"
                        << "  information that is used to create the bot client. Any WickrIO bot client\n"
                        << "  that associates with integration software may have more values that need to\n"
                        << "  be specified in the config file. Details of these values can be found in the\n"
                        << "  sample config file.\n";
            } else if (cmd == "delete") {
                qDebug().nospace() << "CONSOLE:Delete command usage: delete <client number>\n"
                        << "  The delete command is used to delete an existing client. The <client number>\n"
                        << "  is the number shown on the 'list' command associated with the WickrIO bot\n"
                        << "  client that you want to delete. The client's information will be removed\n"
                        << "  from all associated databases. You will be prompted to continue if the\n"
                        << "  associated database record identifies it as running. In that case make sure\n"
                        << "  the process for that bot is actually NOT running (i.e. ps -aef | grep <name\n"
                        << "  of bot>). You should NOT delete a running WickrIO bot client. Please contact\n"
                        << "  Wickr support if you have problems with stopping a running bot. You will also\n"
                        << "  be prompted to delete all the files and directories associated with the bot\n"
                        << "  to be deleted. It is advised to do so, and is the default response.\n";
            } else if (cmd == "help") {
                qDebug().nospace() << "CONSOLE:Help command usage: help [command ...]\n"
                        << "  Use the help command to display the list of commands you can use at this\n"
                        << "  command level. You can also supply a list of one or more commands on the same\n"
                        << "  line as the help command which the help command will display detailed\n"
                        << "  information for.\n";
            } else if (cmd == "integration") {
                qDebug().nospace() << "CONSOLE:Integration command usage: integration\n"
                        << "  The integration command is used to create, configure and maintain the\n"
                        << "  integrations that can be associated with the WickrIO bots clients. When\n"
                        << "  you submit the integration command you will enter the integrations level.\n"
                        << "  You can get details of the integration commands at that level.\n";
            } else if (cmd == "list") {
                qDebug().nospace() << "CONSOLE:List command usage: list\n"
                        << "  The list command will display the list of WickrIO bot clients that are\n"
                        << "  currently configured. The output from the list command will show the name of\n"
                        << "  the bot and then information associated with the bot, like the run state of\n"
                        << "  the bot, the name of an integration that is associated with the bot, if\n"
                        << "  that integration needs to be updated.\n";
            } else if (cmd == "modify") {
                qDebug().nospace() << "CONSOLE:Modify command usage: modify <client number>\n"
                        << "  The modify command is used to make changes to an existing WickrIO bot client.\n"
                        << "  The <client number> is the number shown on the 'list' command associated with\n"
                        << "  the WickrIO bot client that you want to modify. This command should only be\n"
                        << "  performed on a WickrIO bot that is not running. When you run this command you\n"
                        << "  will be prompted to enter the same fields that you did while creating the\n"
                        << "  WickrIO bot, except for the name.\n";
            } else if (cmd == "pause") {
                qDebug().nospace() << "CONSOLE:Pause command usage: pause <client number> [-force]\n"
                        << "  The pause command is used to stop (pause) a WickrIO bot client. The <client\n"
                        << "  number> is the number shown on the 'list' command associated with the WickrIO\n"
                        << "  bot client that you want to pause. The WickrIO bot client should be in the\n"
                        << "  running state to perform this command. Run the 'list' command after this to\n"
                        << "  see the state of the WickrIO bot client to change to the paused state.\n"
                        << "  NOTE: If the WickrIO bot client is showing an invalid running state and you\n"
                        << "  can confirm it is (by doing a ps -aef <client name>) then you can use the\n"
                        << "  -force option to force the WickrIO bot client into the paused state.\n";
            } else if (cmd == "quit") {
                qDebug().nospace() << "CONSOLE:Quit command usage: quit\n"
                        << "  The quit command is used to exit this program.\n"
                        << "  WARNING: leaving this program will also put all of the WickrrIO bot clients\n"
                        << "  into the paused state.\n";
            } else if (cmd == "start") {
                qDebug().nospace() << "CONSOLE:Start command usage: start <client number> [-force]\n"
                        << "  The start command is used to start a specific WickrIO bot client. The <client\n"
                        << "  number> is the number shown on the 'list' command associated with the WickrIO\n"
                        << "  bot client that you want to start.\n"
                        << "  NOTE: If the WickrIO bot client is showing an invalid running state and you\n"
                        << "  can confirm it is (by doing a ps -aef <client name>) then you can use the\n"
                        << "  -force option to force the WickrIO bot client into the started state.\n";
            } else if (cmd == "upgrade") {
                qDebug().nospace() << "CONSOLE:Upgrade command usage: upgrade <client number>\n"
                        << "  The upgrade command is used to upgrade the integration software associated\n"
                        << "  with an existing WickrIO bot client. The <client numer> is the number shown\n"
                        << "  on the 'list' command associated with the WickrIO bot client that you want to\n"
                        << "  upgrade the integration software for. If there are version numbers associated\n"
                        << "  with the integration software and the interation software has a new version\n"
                        << "  there will be an indication next to the WickrIO bot client saying it needs to\n"
                        << "  be upgraded. You can always performe an upgrade even if the upgarde is not\n"
                        << "  indicated.\n";
            } else if (m_root && cmd == "version") {
                qDebug().nospace() << "CONSOLE:Version command usage: version\n"
                        << "  This command will display the version number of this software.\n";
            } else if (m_root && cmd == "welcome") {
                qDebug().nospace() << "CONSOLE:Welcome command usage: welcome [on|off]\n"
                        << "  The welcome command with out arguments will display the welcome message. If\n"
                        << "  the \"on\" argument is entered, then the welcome command will be displayed every\n"
                        << "  time the WickrIO console is started. If the \"off\" argument is entered then the\n"
                        << "  welcome message will not be shown when the WickrIO console is started.\n";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    } else {
        qDebug() << "CONSOLE:Commands:";
        qDebug() << "CONSOLE:  add         - adds a new client";
        if (!m_root) qDebug() << "CONSOLE:  back        - leave the clients setup";
        qDebug() << "CONSOLE:  config      - process a configuration file";
        qDebug() << "CONSOLE:  delete <#>  - deletes client with the specific index";
        qDebug() << "CONSOLE:  help or ?   - shows supported commands";
        qDebug() << "CONSOLE:  integration - bot integrations menu";
        qDebug() << "CONSOLE:  list        - shows a list of clients";
        qDebug() << "CONSOLE:  modify <#>  - modifies a client with the specified index";
        qDebug() << "CONSOLE:  pause <#>   - pauses the client with the specified index";
        qDebug() << "CONSOLE:  quit        - leaves this program";
        qDebug() << "CONSOLE:  start <#>   - starts the client with the specified index";
        qDebug() << "CONSOLE:  upgrade <#> - upgrade integration software for client";
        if (m_root) qDebug() << "CONSOLE:  version     - display the version number of this software";
        if (m_root) qDebug() << "CONSOLE:  welcome     - display the welcome message";
    }
}

bool CmdClient::processCommand(QStringList cmdList, bool &isquit)
{
    bool retVal = true;
    bool bForce = false;
    isquit = false;

    QString cmd = cmdList.at(0).toLower();
    int clientIndex;

    if (cmd == "?" || cmd == "help") {
        QStringList args;
        if (cmdList.size() > 1) {
            args = cmdList;
            args.removeAt(0);
        }
        processHelp(args);
        return true;
    }

    if (m_root && cmd == "welcome") {
        if (cmdList.size() > 1) {
            QString option = cmdList.at(1);
            if (option == "on") {
                m_showWelcomeMsg = true;
            } else if(option == "off") {
                m_showWelcomeMsg = false;
            } else {
                qDebug() <<"CONSOLE:Invalid option for the welcome command. Should be 'on' or 'off'";
            }

            m_operation->m_settings->beginGroup(WBSETTINGS_HELP_HEADER);
            m_operation->m_settings->setValue(WBSETTINGS_HELP_SHOW_WELCOME, m_showWelcomeMsg);
            m_operation->m_settings->endGroup();
            m_operation->m_settings->sync();
            return true;
        }
    }

    // Convert the second argument to an integer, for the client index commands
    if (cmdList.size() > 1) {
        bool ok;
        clientIndex = cmdList.at(1).toInt(&ok);
        if (!ok) {
            qDebug() << "CONSOLE:Client Index is not a number!";
            return true;
        }

        // See if the force option is set
        if (cmdList.size() == 3) {
            if (cmdList.at(2) == "-force" || cmdList.at(2) == "force") {
                bForce = true;
            }
        }
    } else {
        clientIndex = -1;
    }

    if (cmd == "add") {
        addClient();
    } else if (cmd == "back" && !m_root) {
        retVal = false;
    } else if (cmd == "config") {
        configClients();
    } else if (cmd == "delete") {
        if (clientIndex == -1) {
            qDebug() << "CONSOLE:Usage: delete <index>";
        } else {
            deleteClient(clientIndex);
        }
    } else if (cmd == "integration") {
        retVal = m_cmdIntegration.runCommands();
        updateIntegrationVersion();
    } else if (cmd == "list") {
        listClients();
    } else if (cmd == "modify") {
        if (clientIndex == -1) {
            qDebug() << "CONSOLE:Usage: modify <index>";
        } else {
            modifyClient(clientIndex);
        }
    } else if (cmd == "pause") {
        if (clientIndex == -1) {
            qDebug() << "CONSOLE:Usage: pause <index>";
        } else {
            pauseClient(clientIndex, bForce);
        }
    } else if (cmd == "ports") {
        listInboundPorts();
    } else if (cmd == "quit") {
        retVal = false;
        isquit = true;
    } else if (cmd == "start") {
        if (clientIndex == -1) {
            qDebug() << "CONSOLE:Usage: start <index>";
        } else {
            startClient(clientIndex, bForce);
        }
    } else if (cmd == "upgrade") {
        if (clientIndex == -1) {
            qDebug() << "CONSOLE:Usage: upgrade <index>";
        } else {
            upgradeClient(clientIndex);
        }
    } else if (cmd == "version" && m_root) {
        qDebug().nospace().noquote() << "CONSOLE:version: " << getVersionString();
    } else if (m_root && cmd == "welcome") {
        welcomeMessage();
    } else {
        qDebug() << "CONSOLE:" << cmd << "is not a known command!";
    }
    return retVal;
}

void CmdClient::welcomeMessage()
{

    qDebug().nospace() << "CONSOLE:Welcome to the WickrIO Console program.\n"
                       << "This console is used to maintain the WickrIO bot clients. The commands of this\n"
                       << "program support adding, modifying, deleting, starting and stopping the WickrIO\n"
                       << "bot clients. You can also import integrations that you want to associate with\n"
                       << "any of the bot clients you create. Before you add a WickrIO bot client to this\n"
                       << "system you will have to use the Wickr Admin Console to create the client. Once\n"
                       << "you have done that you can use the 'add' command to add it to this system. You\n"
                       << "can use one of the included integrations (hubot, web_interface, file_bot, etc)\n"
                       << "or create your own. Details on how to create a bot integration are documented.\n"
                       << "There are several commands that will help you along the way, the 'help' command\n"
                       << "can be used to get details on each of the commands. The basic steps to create a\n"
                       << "bot client are the following:\n\n"
                       << "  1. Run the Wickr Admin Console.\n"
                       << "  2. Select 'Users' tab in the Network you want to add the bot client\n"
                       << "  3. Go to the Active Bots section\n"
                       << "  4. Add the appropriate values for your new bot, and select 'Create':\n"
                       << "     - bot display name: displayed on the Wickr clients\n"
                       << "     - bot username: need this to add the bot into WickrIO console\n"
                       << "     - password: the password of the bot client\n"
                       << "  5. Enter the 'add' command in this console.\n"
                       << "  6. When prompted enter the bot username, and password. NOTE: the password is\n"
                       << "     not saved to disk.\n"
                       << "  7. Select if you want to use auto login capability. NOTE: you will be\n"
                       << "     prompted the first time you start the bot client. The password will be\n"
                       << "     used to generate a key, but will not be saved. If you do not use auto\n"
                       << "     login then each time the bot client is started you will have to enter\n"
                       << "     the password.\n"
                       << "  8. A list of bot integrations will be shown. Bot integrations are software\n"
                       << "     modules that connect to the bot client and provide specific features\n"
                       << "     using the Wickr messaging capaibilities. If you wish to use one of the\n"
                       << "     supported bot integrations then enter 'yes'\n"
                       << "  9. If you selected yes to the bot integrations, enter the bot integration\n"
                       << "     you wish to use. You will be prompted for any values that are needed to\n"
                       << "     configure that bot integration. The bot integration software will be\n"
                       << "     installed for your new bot.\n"
                       << " 10. The bot will be installed and configured. If you enter the 'list' command\n"
                       << "     it should be at the bottom of the list. Use the 'start' command to start\n"
                       << "     your bot.\n\n"
                       << "Another option is to use the 'config' command to import a ini file that contains\n"
                       << "a list of bot clients and their associated configuration values. See the help\n"
                       << "for the 'config' command for more details.\n";
}

/**
 * @brief CmdClient::runCommands
 * This function handles the setup of the different clients running on the system.
 * There can be multiple clients on a system. The user add new clients or modify
 * the configuration of existing clients.  Only clients in the paused state can be
 * modified. The user can start or pause a client as well from this function.
 */
bool CmdClient::runCommands(const QStringList& options, QString commands)
{
    // Default to advanced configuration (for now)
    m_basicConfig = false;

    for (QString option : options) {
        if (option == "-basic") {
            m_basicConfig = true;
        } else if (option == "-advanced") {
            m_basicConfig = false;
        } else if (option == "-root") {
            m_root = true;
        }
    }

    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    // Get the current SSL Settings
    if (m_operation->m_settings->childGroups().contains(WBSETTINGS_SSL_HEADER)) {
        // Get the email settings from the settings file
        m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
        m_sslSettings.readFromSettings(m_operation->m_settings);
        m_operation->m_settings->endGroup();
    } else {
        m_sslSettings.sslKeyFile = "";
        m_sslSettings.sslCertFile = "";
    }

    // See if the welcome message should be shown or not
    if (m_operation->m_settings->childGroups().contains(WBSETTINGS_HELP_HEADER)) {
        m_operation->m_settings->beginGroup(WBSETTINGS_HELP_HEADER);
        m_showWelcomeMsg = m_operation->m_settings->value(WBSETTINGS_HELP_SHOW_WELCOME, true).toBool();
        m_operation->m_settings->endGroup();
    } else {
        m_showWelcomeMsg = true;
    }

    // Get the data from the database
    m_clients = m_operation->m_ioDB->getClients();
    bool isQuit;

    // show the welcome message if it should be and hasn't been shown
    if (m_showWelcomeMsg && !m_welcomeMsgShown) {
        m_welcomeMsgShown = true;
        welcomeMessage();
    }

    if (commands.isEmpty()) {
        //TODO: if there are no clients then print out something, if this is the first time through as well

        while (true) {
            QString prompt;
            if (m_root) {
                prompt = "Enter command:";
            } else {
                prompt = "Enter client command:";
            }
            QString line = getCommand(prompt);

            if (line.length() > 0) {
                QStringList args = line.split(" ");
                if (!processCommand(args, isQuit)) {
                    break;
                }
            }
        }
    } else {
        QStringList cmds = commands.trimmed().split(" ");
        processCommand(cmds, isQuit);
    }
    return !isQuit;
}

void CmdClient::status()
{
    listClients();
}

void CmdClient::listInboundPorts()
{
    QStringList ports;

    // Update the list of clients
    m_clients = m_operation->m_ioDB->getClients();
    for (WickrBotClients *client : m_clients) {
        // If there is a port assigned and it is not for an integration
        if (client->port != 0 && client->botType.isEmpty()) {
            ports.append(QString::number(client->port));
        }
    }

    if (ports.length() > 0) {
        qDebug().noquote().nospace() << "CONSOLE:Warning: the following incoming ports are being used: " << ports.join(',') << "\n";
    }
}

/**
 * @brief CmdClient::listClients
 * This funciton will print out a list of the current clients from the database.
 */
void CmdClient::listClients()
{
    // Update the list of clients
    m_clients = m_operation->m_ioDB->getClients();

    if (m_clients.size() == 0) {
        qDebug() << "CONSOLE:There are no clients currently configured!";
        return;
    }
    qDebug() << "CONSOLE:Current list of clients:";

    int cnt=0;
    for (WickrBotClients *client : m_clients) {
        QString processName = WBIOServerCommon::getClientProcessName(client);
        QString clientState = WickrIOConsoleClientHandler::getActualProcessState(processName, client->name, m_operation->m_ioDB);
        WickrIOConsoleUser cUser;
        QString consoleUser;

        if (m_operation->m_ioDB->getConsoleUser(client->console_id, &cUser)) {
            consoleUser = cUser.user;
        } else {
            consoleUser = "Not set";
        }

        QString botTypeString;
        bool needsUpgrade = false;
        if (!client->botType.isEmpty()) {
            unsigned newBotVer = m_integrationVersions.value(client->botType, 0);
            unsigned curBotVer = 0;

            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(client->botType);
            QFile curHubotVersionFile(QString("%1/VERSION").arg(destPath));
            if (curHubotVersionFile.exists()) {
                curBotVer = getVersionNumber(&curHubotVersionFile);
                if (curBotVer > 0) {
                    if (curBotVer < newBotVer) {
                        needsUpgrade = true;
                    }
                }
            }

            if (needsUpgrade) {
                botTypeString = QString(", Integration=%1 (Needs Upgrade!)").arg(client->botType);
            } else {
                botTypeString = QString(", Integration=%1").arg(client->botType);
            }
        }

        // If there is a port configured then create the port string
        QString portString;
        if (client->port != 0) {
            if (client->apiKey.isEmpty()) {
                portString = QString(", Port=%1").arg(client->port);
            } else {
                portString = QString(", Port=%1, APIKey=%2").arg(client->port).arg(client->apiKey);
            }
        }

        QString data;
        if (m_basicConfig) {
            data = QString("CONSOLE: client[%1] %2%3, State=%4%5")
                .arg(cnt++)
                .arg(client->user)
                .arg(portString)
                .arg(clientState)
                .arg(botTypeString);
        } else {
            data = QString("CONSOLE: client[%1] %2%3, Binary=%4, State=%5, ConsoleUser=%6%7")
                .arg(cnt++)
                .arg(client->user)
                .arg(portString)
                .arg(client->binary)
                .arg(clientState)
                .arg(consoleUser)
                .arg(botTypeString);
        }
        qDebug() << qPrintable(data);
    }
}

/**
 * @brief CmdClient::chkClientsUserExists
 * Check if the input user is already used by one of the client records
 * @param name
 * @return
 */
bool CmdClient::chkClientsUserExists(const QString& user)
{
#if 0
    for (WickrBotClients *client : m_clients) {
        if (client->user == user) {
            qDebug() << "CONSOLE:The input user name is NOT unique!";
            return true;
        }
    }
    return false;
#else
    Q_UNUSED(user);

    // Allow duplicates for now
    return false;
#endif
}

/**
 * @brief CmdClient::chkClientsInterfaceExists
 * Check if the input interface and port combination is already used
 * by one of the client records
 * @param name
 * @return
 */
bool CmdClient::chkClientsInterfaceExists(const QString& iface, int port)
{
    // TODO: Make sure not using the same port as the console server
    if( port < 0){
        qDebug() << "CONSOLE:Port number too small. Must be greater than or equal to zero";
        return false;
    }
    if( port > 65536){
        qDebug() << "CONSOLE:Port number too large. Must be less than 65536";
        return false;
    }
    for (WickrBotClients *client : m_clients) {
        if (client->port == port && (client->iface == iface || client->iface == "localhost" || iface == "localhost" )) {
            qDebug() << "CONSOLE:The input interface and port are NOT unique!";
            return true;
        }
    }
    return false;
}

bool CmdClient::getClientValues(WickrBotClients *client, const QMap<QString,QString>& keyValuePairs, bool fromConfig)
{
    bool quit = false;
    QString temp;
    bool getInterfaceInfo = false;

    // Do not prompt for the username if from a config file
    if (!fromConfig) {
        // Get a unique user name
        do {
            temp = getNewValue(client->user, tr("Enter the user name"));

            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return false;
            }

            // Allow the user to re-use the same name if it was previously set
            if (! client->user.isEmpty()  && client->user == temp) {
                break;
            }
        } while (chkClientsUserExists(temp));
        client->user = temp;
    }

    // Get the password
    while (true) {
        client->password = getPassword(tr("Enter the password:"));
        if (client->password.isEmpty() || client->password.length() < 4) {
            qDebug() << "CONSOLE:Password should be at least 4 characters long!";
        } else {
            break;
        }
    }

    // The client name will be the username.  Replace the "@" character with the "_"
    client->name = client->user;
    client->name.replace("@", "_");

    // If not from config file, get the binary type
    if (!fromConfig) {
        // Determine the client type
        QStringList possibleClientTypes = WBIOServerCommon::getAvailableClientApps();
        QString binary;
        if (possibleClientTypes.length() > 1) {
            temp = getNewValue(client->binary, tr("Enter the client type"), CHECK_LIST, possibleClientTypes);
            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return false;
            }
            binary = temp;
        } else {
            binary = possibleClientTypes.at(0);
        }
        client->binary = binary;
    }

    // Determine if the BOT has an executable that will create/provision the user

    QString provisionApp = WBIOServerCommon::getProvisionApp(client->binary);

    if (provisionApp != "") {
        QStringList arguments;
        bool    botNotCreated = true;
        int     returnCode;

        while (botNotCreated) {
            arguments.append(client->user);
            arguments.append(client->password);

            m_exec = new QProcess();

            connect(m_exec, SIGNAL(finished(int)), this, SLOT(slotCmdFinished));
    //        connect(m_exec, SIGNAL(finished(int, QProcess::readyReadStandardOutput)), this, SLOT(slotCmdOutputRx));

            //Tests that process starts and closes alright
            m_exec->start(provisionApp, arguments);

            if (m_exec->waitForStarted(-1)) {
                // Continue reading the data until EOF reached
                while(m_exec->waitForReadyRead()) {
                    QByteArray data;
                    data = m_exec->readAll();

                    // Output the data
                    qDebug().noquote().nospace() << "CONSOLE:" << data;
                }
                m_exec->waitForFinished(-1);

                returnCode = m_exec->exitCode();
                qDebug() << "CONSOLE:Return code from provision is:" << returnCode;
            } else {
                QByteArray errorout = m_exec->readAllStandardError();
                if (!errorout.isEmpty()) {
                    qDebug() << "ERRORS" << errorout;
                }
                qDebug() << "Exit code=" << m_exec->exitCode();
                returnCode = WIOPROVISION_FAILED;
            }
            m_exec->close();
            m_exec->deleteLater();
            m_exec = nullptr;

            // Process the return code
            if (returnCode == WIOPROVISION_BAD_CREDENTIALS) {
                qDebug() << QString("CONSOLE:User exists already, password entered seems to be invalid!");
                while (true) {
                    QString temp = getNewValue("yes", tr("Do you want to try a new password?"), CHECK_BOOL);
                    if (temp.toLower() == "yes" || temp.toLower() == "y") {
                        break;
                    }
                    if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
                        return false;
                    }

                    // Check if the user wants to quit the action
                    if (handleQuit(temp, &quit) && quit) {
                        return false;
                    }
                }
                while (true) {
                    client->password = getPassword(tr("Enter the password:"));
                    if (client->password.isEmpty() || client->password.length() < 4) {
                        qDebug() << "CONSOLE:Password should be at least 4 characters long!";
                    } else {
                        break;
                    }
                }
            } else if (returnCode == WIOPROVISION_FAILED) {
                qDebug().noquote() << QString("CONSOLE:Failed to provision or login bot client %1!").arg(client->user);
                return false;
            } else if (returnCode == WIOPROVISION_USER_NOT_FOUND) {
                qDebug().noquote() << QString("CONSOLE:%1 does not seem to exist. Please verify in the Admin Console.!").arg(client->user);
                return false;
            } else {
                break;
            }
        }

    } else {
    }

    // if not from config file, see if the user wants to use auto login capability
    if (!fromConfig) {
        // See if the user wants to use the autologin capability
        qDebug() << "CONSOLE:The autologin capability allows you to start a bot without having to enter the password,\nafter the initial login.";
        qDebug() << "CONSOLE:Warning: The bot client's password is NOT saved to disk, but it is less secure.";
        while (true) {
            QString temp = getNewValue("yes", tr("Do you want to use autologin?"), CHECK_BOOL);
            if (temp.toLower() == "yes" || temp.toLower() == "y") {
                client->m_autologin = true;
                break;
            }
            if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
                client->m_autologin = false;
                break;
            }

            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return false;
            }
        }
    }

#if 0 // Not supporting http/https in the core anymore
    // If not from mconfig file, get the interface information
    if (!fromConfig) {
        if (!m_basicConfig) {
            getInterfaceInfo = true;
        } else {
            // For basic configuration prompt the user if they want to add HTTP interface (default: no)
            while (true) {
                QString temp = getNewValue("no", tr("Do you want to use the HTTP API?"), CHECK_BOOL);
                if (temp.toLower() == "yes" || temp.toLower() == "y") {
                    getInterfaceInfo = true;
                    break;
                }
                if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
                    getInterfaceInfo = false;
                    break;
                }

                // Check if the user wants to quit the action
                if (handleQuit(temp, &quit) && quit) {
                    return false;
                }
            }
        }

        if (getInterfaceInfo) {

            // Generate the API Key, with a random value
            client->apiKey = WickrIOTokens::getRandomString(16);

    #if 0 // Defaulting to localhost
            // Get the list of possible interfaces
            QStringList ifaceList = WickrIOConsoleClientHandler::getNetworkInterfaceList();
    #endif

            // Get a unique interface and port pair
            while (true) {
        #if 1   // Will default to "localhost"
                client->iface = "localhost";
                QString ifaceinput = "localhost";
        #else
                QString ifaceinput = getNewValue(client->iface, tr("Enter the interface (list to see possible interfaces)"));

                // Check if the user wants to quit the action
                if (handleQuit(ifaceinput, &quit) && quit) {
                    return false;
                }

                if (ifaceinput.toLower() == "list" || ifaceinput == "?") {
                    foreach (QString iface, ifaceList) {
                        qDebug() << "CONSOLE:" << iface;
                    }
                    continue;
                }

                if (!ifaceList.contains(ifaceinput)) {
                    qDebug() << "CONSOLE:" << ifaceinput << "not found in the list of interfaces!";
                    continue;
                }
        #endif

                QString port = getNewValue(QString::number(client->port), tr("Enter the IP port number"), CHECK_INT);

                // Check if the user wants to quit the action
                if (handleQuit(port, &quit) && quit) {
                    return false;
                }

                int portinput = port.toInt();

                // Check if used the same values, if so continue on to the next field
                if (!client->iface.isEmpty() && (client->iface == ifaceinput && client->port == portinput)) {
                    break;
                }

                if (chkClientsInterfaceExists(ifaceinput, portinput)) {
                    qDebug() << "CONSOLE:Port number is used already!";
                    continue;
                }

                client->iface = ifaceinput;
                client->port = portinput;

                break;
            }

            // Get the interface type (HTTP or HTTPS)
            while (true) {
                QString ifacetype;
                ifacetype = getNewValue(client->getIfaceTypeStr(), tr("Enter the interface type"));
                if (ifacetype.toLower() == "https") {
                    client->isHttps = true;
                    if (m_sslSettings.sslKeyFile.isEmpty() || m_sslSettings.sslCertFile.isEmpty()) {
                        qDebug() << "CONSOLE:WARNING: SSL has not been setup! Go to the Advanced settings first!";
                    }
                } else if (ifacetype.toLower() == "http") {
                    client->isHttps = false;
                } else {
                    qDebug() << "CONSOLE:Invalid interface type, enter either HTTP or HTTPS";
                    continue;

                }
                break;
            }

            // Get the SSL Key and Cert values
            if (client->isHttps) {
                client->sslKeyFile = m_sslSettings.sslKeyFile;
                client->sslCertFile = m_sslSettings.sslCertFile;
            }

    #if 0 // Need to figure out how we want to set console users
            QList<WickrIOConsoleUser *> cusers = m_operation->m_ioDB->getConsoleUsers();
            if (cusers.length() > 0) {
                int curindex = -1;
                QString temp;
                if (client->console_id != 0) {
                    int cnt=0;
                    for (WickrIOConsoleUser *cuser : cusers) {
                        if (cuser->id == client->console_id) {
                            curindex = cnt;
                            break;
                        }
                        cnt++;
                    }
                } else {
                    curindex = -1;
                }
                temp = getNewValue(curindex == -1 ? "n" : "y", tr("Associate with a console user?"));
                if (handleQuit(temp, &quit) && quit) {
                } else if (temp == "y") {
                    // Associate with a console user
                    while (true) {
                        qDebug() << "CONSOLE:Possible console user choices:";
                        int cnt=0;
                        for (WickrIOConsoleUser *cuser : cusers) {
                            QString data = QString("CONSOLE:  index=%1, Name=%2").arg(cnt++).arg(cuser->user);
                            qDebug() << qPrintable(data);
                        }
                        temp = getNewValue(curindex == -1 ? "" : QString::number(curindex), tr("Enter the index of the console user"), CHECK_INT);
                        if (handleQuit(temp, &quit) && quit) {
                            break;
                        }
                        int inputIndex = temp.toInt();
                        if (inputIndex < 0 || inputIndex >= cusers.length()) {
                            qDebug() << "CONSOLE:Invalid index value entered!";
                        } else {
                            curindex = inputIndex;
                            break;
                        }
                    }
                } else {
                    curindex = -1;
                }

                // If the user is NOT quitting then update the console ID
                if (!quit) {
                    if (curindex == -1) {
                        client->console_id = 0;
                    } else {
                        client->console_id = cusers.at(curindex)->id;
                    }
                }

                // Cleanup the allocated memory
                for (WickrIOConsoleUser *cuser : cusers) {
                    delete cuser;
                }
            }
    #endif

            // Inbox handling can be turned off for Welcome Bots (only)
            //check whether type is welcome_bot to see if need to find parser
            if(client->binary.startsWith( "welcome_bot")){
                while (true) {
                    QString handleInbox;
                    QStringList trueFalseChoices;
                    trueFalseChoices << "true" << "false";
                    handleInbox = getNewValue(client->getHandleInboxStr(), tr("Does client support inbox handling?"), CHECK_LIST, trueFalseChoices);
                    if (handleInbox.toLower() == "true") {
                        client->m_handleInbox = true;
                    } else if (handleInbox.toLower() == "false") {
                        client->m_handleInbox = false;
                    } else {
                        qDebug() << "CONSOLE:Invalid input, enter either true or false";
                        continue;

                    }
                    break;
                }
            } else {
                client->m_handleInbox = true;
            }
        }
    }
#endif

    // If not from a config file, determine if the user wants a integration and which one
    if (!fromConfig) {
        /*
         * setup an integration bot, if the users desires to use one
         */
        client->botType = QString();
        QString rmBotType;

        // Check if the integrations directory exists
        QList<WBIOBotTypes *>botTypes = WBIOServerCommon::getBotsSupported(client->binary, false);
        if (botTypes.length() > 0) {
            QList<WBIOBotTypes *>supportedIntegrations;
            QStringList possibleBotTypes;

            for (WBIOBotTypes *botType : botTypes) {
                /* If this bot is installed and it doesn't need HTTP API,
                 * or needs HTTP API and HTTP iface is configured
                 */
                if ((getInterfaceInfo && botType->useHttpApi()) || !botType->useHttpApi()) {
                    supportedIntegrations.append(botType);
                    possibleBotTypes.append(botType->m_name);
                }
            }

            if (supportedIntegrations.length() > 0) {
                QString hasIntBot;
                hasIntBot = client->botType.isEmpty() ? "no" : "yes";

                qDebug().noquote().nospace() << "CONSOLE:The following bot types are available: " << possibleBotTypes.join(',');

                // If the user wants to connect the client to an integration bot
                temp = getNewValue(hasIntBot, tr("Do you want to connect to a integration bot?"), CHECK_BOOL);
                if (temp == "yes") {

                    temp = getNewValue(client->botType, tr("Enter the bot type"), CHECK_LIST, possibleBotTypes);
                    // Check if the user wants to quit the action
                    if (handleQuit(temp, &quit) && quit) {
                        return false;
                    }
                    if (temp != "none") {
                        // if the bottype has changed then remove the old software
                        if (client->botType != temp)
                            rmBotType = client->botType;

                        client->botType = temp;
                    } else {
                        rmBotType = client->botType;
                        client->botType = QString();
                    }
                } else {
                    rmBotType = client->botType;
                    client->botType = QString();
                }
            }
        }

        // If there is a integration bot software to remove then do so
        if (!rmBotType.isEmpty()) {
            // Getting rid of the integration bot, will have to remove the current installation directory
            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(rmBotType);
            if (!destPath.isEmpty()) {
                qDebug().noquote().nospace() << "CONSOLE:Removing software installation for previous integration " << rmBotType;
                QDir destDir(destPath);
                if (destDir.exists()) {
                    if (!destDir.removeRecursively())
                        qDebug().noquote().nospace() << "CONSOLE:Could not remove " << destPath;
                }
            }
        }
    }

    /*
     * Need to configure the bot if one was selected
     */
    if (! client->botType.isEmpty()) {
        // Get the software into the appropriate location
        QString swPath = WBIOServerCommon::getBotSoftwarePath(client->botType);
        if (!swPath.isEmpty()) {
            qDebug() << "CONSOLE:**********************************************************************";
            qDebug().noquote().nospace() << "CONSOLE:Begin setup of " << client->botType << " software for " << client->user;

            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(client->botType);

            // Create the directory for the integration software
            QDir destDir(destPath);
            if (!destDir.exists()) {
                if (!destDir.mkpath(destPath)) {
                    qDebug() << "CONSOLE:Failed to create directory for integration bot software!";
                    return false;
                }
            }

            // First copy the software to the client directory
            if (! integrationCopySW(client, swPath, destPath)) {
                return false;
            }

            // Second peform the installer if one does exist
            if (! integrationInstall(client, destPath)) {
                return false;
            }

            // Third peform the configure if one does exist
            if (! integrationConfigure(client, destPath, keyValuePairs)) {
                return false;
            }

            qDebug().noquote().nospace() << "CONSOLE:End of setup of " << client->botType << " software for " << client->name;
            qDebug() << "CONSOLE:**********************************************************************";

        }
    }


    return !quit;
}

bool
CmdClient::getAuthValue(WickrBotClients *client, bool basic, QString& authValue)
{
    WickrIOTokens token;
    QString basicAuthString;

    // get the id of the console user to use for authentication
    if (client->console_id == 0) {
        QList<WickrIOConsoleUser *> cusers;
        cusers = m_operation->m_ioDB->getConsoleUsers();

        if (cusers.length() == 0) {
            qDebug() << "CONSOLE:There are not console users defined.  Please create a console user and a token.";
            return false;
        }

        qDebug() << "CONSOLE:You will need to select a console user to use their authentication token";
        qDebug() << "CONSOLE:Please select from the following list of console users:";

        int curindex = -1;
        QString temp;
        bool quit=false;
        // Associate with a console user
        while (true) {
            qDebug() << "CONSOLE:Possible console user choices:";
            int cnt=0;
            for (WickrIOConsoleUser *cuser : cusers) {
                QString data = QString("CONSOLE:  index=%1, Name=%2").arg(cnt++).arg(cuser->user);
                qDebug() << qPrintable(data);
            }
            temp = getNewValue(curindex == -1 ? "" : QString::number(curindex), tr("Enter the index of the console user"), CHECK_INT);
            if (handleQuit(temp, &quit) && quit) {
                break;
            }
            int inputIndex = temp.toInt();
            if (inputIndex < 0 || inputIndex >= cusers.length()) {
                qDebug() << "CONSOLE:Invalid index value entered!";
            } else {
                curindex = inputIndex;
                break;
            }
        }

        if (quit || curindex == -1)
            return false;

        WickrIOConsoleUser *cuser = cusers.at(curindex);
        client->console_id = cuser->id;

        basicAuthString = QString("%1:%2").arg(cuser->user).arg(cuser->password);

        // Cleanup the allocated memory
        for (WickrIOConsoleUser *cuser : cusers) {
            delete cuser;
        }
    } else if (basic) {
        WickrIOConsoleUser cuser;

        if (!m_operation->m_ioDB->getConsoleUser(client->console_id, &cuser)) {
            qDebug() << "CONSOLE:There was a problem retrieving the console user!";
            return false;
        }
        basicAuthString = QString("%1:%2").arg(cuser.user).arg(cuser.password);
    }

    if (basic) {
        authValue = basicAuthString;
    } else {
        if (! m_operation->m_ioDB->getConsoleUserToken(client->console_id, &token)) {
            qDebug() << "CONSOLE:There is no token associated with that console user!";
            return false;
        }

        // Should set the actual authentication type!
        authValue = token.token;
    }
    return true;
}

/**
 * @brief CmdClient::readLineFromProcess
 * This function will process input from a running process, ignoring the newline character.
 * If the process stops running it will return a false value.  If multiple lines are read
 * in then only one line, up to the newline character, will be returned. Other lines will be
 * returned the next time this function is called.
 * @param process
 * @param line
 * @return
 */
bool
CmdClient::readLineFromProcess(QProcess *process, QString& line)
{
    static QList<QByteArray> savedBytes;

    // If there are bytes from a previous call use them
    if (savedBytes.length() > 0) {
        line = QString(savedBytes.takeFirst());
    } else {
        QThread::sleep(1);

        while (process->state() == QProcess::Running) {
            if (process->waitForReadyRead(250)) {
                QByteArray bytes = process->readAll();

                QList<QByteArray> lines = bytes.split('\n');

                if (lines.size() == 0) {
                    line = "";
                    savedBytes.clear();
                } else {
                    line = QString(lines.takeFirst());
                    savedBytes = lines;
                }
                return true;
            }
        }

        if (process->state() != QProcess::Running) {
            savedBytes.clear();
            return false;
        }
    }

    return true;
}

/**
 * @brief CmdClient::runBotScript
 * This function will run the input script. The script has commands in it to identify if input
 * is required.
 * @param destPath
 * @param configure
 * @return
 */
bool
CmdClient::runBotScript(const QString& destPath, const QString& configure, WickrBotClients *client, const QMap<QString,QString>& keyValuePairs)
{
    // Values associated with the CallbackURL
    QString cbackEndPoint;
    QString cbackPort;

    // Create a file that will contain the keyValuePairs and the client user name
    // The file will be deleted when this function exits
    QTemporaryFile file;
    if (!file.open()) {
        qDebug() << "CONSOLE:Could not create temporary file for starting bot script!";
        return false;
    }
    QStringList args;
    args.append(file.fileName());

    // Write the client name and the keyValuePairs
    QTextStream outputStream(&file);
    outputStream << QString("%1=%2\n").arg(BOTINT_CLIENT_NAME).arg(client->user);
    for (QString key : keyValuePairs.keys()) {
        outputStream << QString("%1=\"%2\"\n").arg(key).arg(keyValuePairs.value(key,""));
    }
    file.close();

    // Create a process to run the configure
    QProcess *runScript = new QProcess(this);

    connect(runScript, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << "CONSOLE:error enum val = " << error;
    });
    runScript->setProcessChannelMode(QProcess::MergedChannels);
    runScript->setWorkingDirectory(destPath);

    runScript->start(configure, args, QIODevice::ReadWrite);

    // Wait for it to start
    if(!runScript->waitForStarted()) {
        qDebug() << QString("CONSOLE:Failed to run %1").arg(configure);
        return false;
    }

    QString bytes;
    while (readLineFromProcess(runScript, bytes)) {
        if (!bytes.isEmpty()) {
            QStringList inputList = bytes.split(":");

            if (inputList.size() > 1 && inputList[0].toLower() == "prompt") {
                bool promptForValue = false;
                if (bytes.contains("WICKRIO_AUTH_TOKEN")) {
                    QString authValue;
#if 0 // Ignoring authentication for now
                    // Get the basic authentication value (user:pw), should use token though
                    if (!getAuthValue(client, true, authValue)) {
                        runScript->close();
                        return false;
                    }
#else
                    authValue = "admin:admin";
#endif

                    authValue.append("\n");
                    runScript->write(authValue.toLatin1());
                } else if (bytes.contains("WICKRIO_SERVER")) {
                    QString server = QString("%1://%2:%3/Apps/%5\n")
                            .arg(client->isHttps ? "https" : "http")
                            .arg(client->iface)
                            .arg(client->port)
                            .arg(client->apiKey);
                    runScript->write(server.toLatin1());
                } else if (bytes.contains("HUBOT_NAME")) {
                    QString botName = QString("%1_hubot\n").arg(client->name);
                    runScript->write(botName.toLatin1());
                } else if (bytes.contains("HUBOT_URL_ENDPOINT")) {
                    cbackEndPoint = QString("/Apps/%1").arg(client->port);
                    QString endpoint = QString("%1\n").arg(cbackEndPoint);
                    runScript->write(endpoint.toLatin1());
                } else if (bytes.contains("HUBOT_URL_PORT")) {
                    QString cbackPortPrompt = QString("Enter the port the %1 integration will listen on").arg(client->botType);
                    cbackPort = getNewValue(cbackPort, cbackPortPrompt);
                    QString outString = QString("%1\n").arg(cbackPort);
                    runScript->write(outString.toLatin1());
                } else {
                    bool foundKey=false;
                    // Check if any of the input keys from the key value pairs match
                    QStringList keys = keyValuePairs.keys();
                    for (QString key : keys) {
                        if (bytes.contains(key)) {
                            foundKey = true;
                            QString valueOutput = QString("%1\n").arg(keyValuePairs.value(key));
                            runScript->write(valueOutput.toLatin1());
                        }
                    }
                    if (!foundKey)
                        promptForValue = true;
                }

                // If there was no known token asked for then prompt to the user
                if (promptForValue) {
                    QString prompt = bytes.right(bytes.length()-7).remove(QRegExp("[\\n\\t\\r]"));     // size of string - sizeof "PROMPT:"
                    QString curVal;
                    prompt = prompt.remove(QRegExp("[\\n\\t\\r]"));
                    QString input = getNewValue(curVal, prompt);

                    input.append("\n");
                    runScript->write(input.toLatin1());
                }
            } else {
                bytes = bytes.remove(QRegExp("[\\n\\t\\r]"));
                qDebug().noquote().nospace() << "CONSOLE:" << bytes;
            }
        }
    }

    // If the callback valuee are set then create the callback url for this client
    if (!cbackEndPoint.isEmpty() && !cbackPort.isEmpty()) {
        client->m_callbackString = QString("http://localhost:%1%2").arg(cbackPort).arg(cbackEndPoint);
    }
    return true;
}

void CmdClient::slotCmdFinished(int)
{
    qDebug() << "process completed";
    m_exec->deleteLater();
    m_exec = nullptr;
}

void CmdClient::slotCmdOutputRx()
{
    QByteArray output = m_exec->readAll();

    qDebug().nospace().noquote() << "CONSOLE:" << output;

    QTextStream s(stdin);
    QString lineInput = s.readLine();
    QByteArray input(lineInput.toUtf8());
    m_exec->write(input);
}


/**
 * @brief CmdClient::addClient
 * This function will add a new client. The user will be prompted for the
 * appropriate fields for the new user.
 */
void CmdClient::addClient()
{
    WickrBotClients *client = new WickrBotClients();
    QMap<QString,QString> clientValues;

    while (true) {
        if (!getClientValues(client, clientValues)) {
            break;
        }

        // Check that the record has already been added (likely during provisioning)
        WickrBotClients *existingClient = m_operation->m_ioDB->getClientUsingUserName(client->user);
        if (existingClient != nullptr) {
            client->id = existingClient->id;
        }

        // Add the new record to the database
        QString errorMsg = WickrIOConsoleClientHandler::addClient(m_operation->m_ioDB, client);
        if (errorMsg.isEmpty()) {
            if (!client->m_callbackString.isEmpty()) {
                WickrIOAppSettings appSetting;
                appSetting.clientID = client->id;
                appSetting.type = DB_APPSETTINGS_TYPE_MSGRECVCALLBACK;
                appSetting.value = client->m_callbackString;
                if (!m_operation->m_ioDB->updateAppSetting(&appSetting)) {
                    qDebug() << "CONSOLE:Failed to create callback connection!";
                }
            }
            qDebug() << "CONSOLE:Successfully added record to the database!";
            break;
        } else {
            qDebug() << "CONSOLE:" << errorMsg;
            bool doItAgain = true;
            while (doItAgain) {
                // If the record was not added to the database then ask the user to try again
                QString response = getNewValue("", tr("Failed to add record, try again?"));
                if (response.isEmpty() || response.toLower() == "n" || response.toLower() == "no") {
                    delete client;
                    return;
                } else if (response.toLower() == "y" || response.toLower() == "yes") {
                    doItAgain = false;
                }
            }
        }
    }
    delete client;

    // Update the list of clients
    m_clients = m_operation->m_ioDB->getClients();
}



/**
 * @brief CmdClient::validateIndex
 * Validate the input index. If invalid output a message
 * @param clientIndex
 */
bool CmdClient::validateIndex(int clientIndex)
{
    if (clientIndex >= m_clients.length() || clientIndex < 0) {
        qDebug() << "CONSOLE:The input client index is out of range!";
        return false;
    }
    return true;
}

/**
 * @brief CmdClient::deleteClient
 * This function is used to delete a client.  The client must be in the paused
 * state first before it can be deleted.
 * @param clientIndex
 */
void CmdClient::deleteClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrBotClients *client = m_clients.at(clientIndex);

        // Make sure the user wants to continue
        QString prompt = QString(tr("Do you really want to remove the client with the name %1")).arg(client->name);
        QString response = getNewValue("", prompt);
        if (response.toLower() == "n" || response.toLower() == "no") {
            return;
        }

        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getClientProcessName(client);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                while (true) {
                    qDebug() << "CONSOLE:The client is running, you should first Pause the client, then delete!";
                    QString response = getNewValue("", "Do you want to continue? (y or n)");
                    if (response.toLower() == "n" || response.toLower() == "no") {
                        return;
                    }
                    if (response.toLower() == "y" || response.toLower() == "yes") {
                        break;
                    }
                }
            }

            qDebug() << "CONSOLE:Deleting client" << client->user;

            if (! m_operation->m_ioDB->deleteClientUsingName(m_clients.at(clientIndex)->name)) {
                qDebug() << "CONSOLE:There was a problem deleting the client!";
            }
            if (! m_operation->m_ioDB->deleteProcessState(processName)) {
                qDebug() << "CONSOLE:There was a problem deleting the client's process record!";
            }

            // Need to cleanup the clients directory and registry (for windows)
            bool deleteFiles=false;
            while (true) {
                qDebug() << "CONSOLE:The files and directories associated with this client should be deleted!";
                QString response = getNewValue("y", "Do you want to remove the associated files for this client? (y or n)");
                if (response.toLower() == "n" || response.toLower() == "no") {
                    break;
                }
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    deleteFiles = true;
                    break;
                }
            }
            if (deleteFiles) {
                QString clientDbDirName = QString(WBIO_CLIENT_WORKINGDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client->name);
                QDir clientDbDir(clientDbDirName);
                if (clientDbDir.exists()) {
                    qDebug() << "CONSOLE:Removing client directory and files!";
                    if (!clientDbDir.removeRecursively()) {
                        qDebug() << "CONSOLE:Failed to remove the client directory and files!";
                    } else {
                        qDebug() << "CONSOLE:Successfully removed the client's directories and files!";
                    }
                }
            }

            // Update the list of clients
            m_clients = m_operation->m_ioDB->getClients();
        }
    }
}

/**
 * @brief CmdClient::modifyClient
 * This function will modify an exist client.
 * @param clientIndex
 */
void CmdClient::modifyClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrBotClients *client;
        WickrBotProcessState state;
        client = m_clients.at(clientIndex);

        QString processName = WBIOServerCommon::getClientProcessName(client);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:Cannot modify a running client!";
            } else {
                while (true) {
                    QMap<QString,QString> clientValues;
                    if (!getClientValues(client, clientValues)) {
                        break;
                    }

                    // update the new record to the database
                    QString errorMsg = WickrIOConsoleClientHandler::addClient(m_operation->m_ioDB, client);
                    if (errorMsg.isEmpty()) {
                        qDebug() << "CONSOLE:Successfully updated record to the database!";
                        break;
                    } else {
                        qDebug() << "CONSOLE:" << errorMsg;
                        // If the record was not updated to the database then ask the user to try again
                        QString response = getNewValue("", tr("Failed to update record, try again?"));
                        if (response.isEmpty() || response.toLower() == "n") {
                            delete client;
                            return;
                        }
                    }
                }
                m_clients = m_operation->m_ioDB->getClients();
            }
        }
    }
}

/**
 * @brief CmdClient::sendIPCCmd
 * This function will send and wait for a successful sent of a message to a
 * client, with the input port number.
 * @param port
 * @param cmd
 * @return
 */
bool CmdClient::sendIPCCmd(const QString& dest, bool isClient, const QString& cmd)
{
    m_clientMsgInProcess = true;
    m_clientMsgSuccess = false;

    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();

    if (ipcSvc == nullptr || ! ipcSvc->sendMessage(dest, isClient, cmd)) {
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
            qDebug() << "CONSOLE:Timed out waiting for stop client message to send!";
        }
    }
    return true;
}

void CmdClient::closeClientIPC(const QString& dest)
{
    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();

    if (ipcSvc != nullptr)
        ipcSvc->closeClientConnection(dest);
}

/**
 * @brief slotGotMessage
 * @param type
 * @param value
 */
void CmdClient::slotReceivedMessage(QString type, QString value)
{
    if (type.toLower() == WBIO_IPCMSGS_STATE) {
        qDebug() << "Client changed state to" << value;
        m_clientStateChanged = true;
        m_clientState = value;
    }
}
/**
 * @brief CmdClient::pauseClient
 * This function is used to pause a running client
 * @param clientIndex
 */
void CmdClient::pauseClient(int clientIndex, bool force)
{
    if (!validateIndex(clientIndex)) {
       qDebug() << "CONSOLE:Invalid client index!";
       return;
    }

    WickrBotClients *client = m_clients.at(clientIndex);
    WickrBotProcessState state;
    QString processName = WBIOServerCommon::getClientProcessName(client);




    if (m_operation->m_ioDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_RUNNING) {
            while (true) {
                QString prompt = QString(tr("Do you really want to pause the client with the name %1")).arg(client->user);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "n" || response.toLower() == "no") {
                    return;
                }
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    break;
                }
            }
        } else {
            if (!force) {
                qDebug() << "CONSOLE:Client must be running to pause it!";
            } else {
                if (! m_operation->m_ioDB->updateProcessState(processName, 0, PROCSTATE_PAUSED)) {
                    qDebug() << "CONSOLE:Failed to change start of client in database!";
                } else {
                    qDebug() << "CONSOLE:Client state was force set to paused.";
                    qDebug() << "CONSOLE:Please verify the client process is not running.";
                }
            }
            return;
        }
    } else {
        qDebug() << "CONSOLE:Could not get the clients state!";
        return;
    }

    // check if there is a integration bot set, if so stop it from running
    if (!client->botType.isEmpty()) {
        QString stopCmd = WBIOServerCommon::getBotStopCmd(client->botType);
        if (!stopCmd.isEmpty()) {
            QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(client->name)
                    .arg(client->botType);
            QString stopFullPath = QString("%1/%2").arg(destPath).arg(stopCmd);

            qDebug() << "**********************************";
            qDebug() << QString("Stopping %1").arg(stopFullPath);

            // Create the argument list for the start script, the client's Wickr ID
            QStringList arguments;
            arguments.append(client->user);

            // Create a process to run the stop script
            QProcess *runBotStopCmd = new QProcess(this);
            runBotStopCmd->setProcessChannelMode(QProcess::MergedChannels);
            runBotStopCmd->setWorkingDirectory(destPath);
            runBotStopCmd->start(stopFullPath, arguments, QIODevice::ReadWrite);

            // Wait for it to start
            if(!runBotStopCmd->waitForStarted()) {
                qDebug() << "Failed to run %1";
            } else {
                QStringList stopOutput;

                while(runBotStopCmd->waitForReadyRead()) {
                    QString bytes = QString(runBotStopCmd->readAll());
                    stopOutput.append(bytes);
                }
            }

            qDebug() << "Done stopping the integration bot!";
            qDebug() << "**********************************";
        }
    }


    if (! sendIPCCmd(client->name, true, WBIO_IPCCMDS_PAUSE)) {
        qDebug() << "CONSOLE:Failed to send message to client!";
    }
    closeClientIPC(client->name);
}

/**
 * @brief CmdClient::startClient
 * This function is used to start a stopped or paused client
 * @param clientIndex
 */
void CmdClient::startClient(int clientIndex, bool force)
{
    if (validateIndex(clientIndex)) {
        WickrBotProcessState state;

        // Get the process state for the Client Server
        if (m_operation->m_ioDB->getProcessState(WBIO_CLIENTSERVER_TARGET, &state)) {
            if (state.state != PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:WARNING: The Client Server is not currently running!";
            }
        } else {
            qDebug() << "CONSOLE:WARNING: The Client Server state is unknown!";
        }

        WickrBotClients *client = m_clients.at(clientIndex);
        QString processName = WBIOServerCommon::getClientProcessName(client);

        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_PAUSED || force) {
                QString prompt = QString(tr("Do you really want to start the client with the name %1")).arg(client->user);
                QString response = getNewValue("", prompt);
                if (response.toLower() != "y" && response.toLower() != "yes") {
                    return;
                }

                // If the password is needed then prompt for it
                bool needPassword = false;
                if (client->m_autologin) {
                    // Check if the database password has been created.
                    // If not then will need the client's password to start.
                    QString clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(m_operation->m_dbLocation).arg(client->name);
                    QString dbKeyFileName = QString("%1/dkd.wic").arg(clientDbDir);
                    QFile dbKeyFile(dbKeyFileName);
                    if (!dbKeyFile.exists()) {
                        needPassword = true;
                    }
                } else {
                    needPassword = true;
                }

                if (needPassword) {
                    QString password;
                    do {
                        password = getPassword("Enter password for this client:");
                    } while (password.isEmpty());

                    // If password is needed Send client information to the Client Server
                    QString clientServerCmd;
                    clientServerCmd = WickrIOIPCCommands::getBotInfoString(m_operation->m_appNm,
                                                                           client->name,
                                                                           client->getProcessName(),
                                                                           password);
                    sendIPCCmd(WBIO_CLIENTSERVER_TARGET, false, clientServerCmd);
                }

                if (! m_operation->m_ioDB->updateProcessState(processName, 0, PROCSTATE_DOWN)) {
                    qDebug() << "CONSOLE:Failed to change start of client in database!";
                    return;
                }

            } else if (state.state == PROCSTATE_DOWN){
                qDebug() << "CONSOLE:Client is already waiting to start. The WickrIO Client Server should change the state to running.";
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
 * @brief CmdClient::upgradeClient
 * This function will modify an exist client.
 * @param clientIndex
 */
void CmdClient::upgradeClient(int clientIndex)
{
    if (validateIndex(clientIndex)) {
        WickrBotClients *client;
        WickrBotProcessState state;
        client = m_clients.at(clientIndex);

        // Make sure there is integration software is installed
        if (client->botType.isEmpty()) {
            qDebug() << "CONSOLE:Client does not have integration software installed!";
            return;
        }

        QString processName = WBIOServerCommon::getClientProcessName(client);
        if (!m_operation->m_ioDB->getProcessState(processName, &state)) {
            qDebug() << "CONSOLE:Cannot client's process state!";
            return;
        }

        if (state.state == PROCSTATE_RUNNING) {
            qDebug() << "CONSOLE:Cannot upgrade a running client! Please stop the client.";
            return;
        }

        unsigned newBotVer = m_integrationVersions.value(client->botType, 0);
        QString  newBotVerString;
        getIntegrationVersionString(newBotVer, newBotVerString);

        unsigned curBotVer = 0;
        QString  curBotVerString;


        QString destPath = QString(WBIO_CLIENT_BOTDIR_FORMAT)
                .arg(WBIO_DEFAULT_DBLOCATION)
                .arg(client->name)
                .arg(client->botType);
        QFile curHubotVersionFile(QString("%1/VERSION").arg(destPath));
        if (curHubotVersionFile.exists()) {
            curBotVer = getVersionNumber(&curHubotVersionFile);
        }
        getIntegrationVersionString(curBotVer, curBotVerString);

        qDebug().noquote().nospace() << "CONSOLE:Upgrading from version " << curBotVerString << " to version " << newBotVerString;
        while (true) {
            // If the record was not updated to the database then ask the user to try again
            QString response = getNewValue("", tr("Okay to proceed?"));
            if (response.toLower() == "n" || response.toLower() == "no") {
                return;
            }
            if (response.toLower() == "y" || response.toLower() == "yes") {
                break;
            }
            qDebug() << "CONSOLE:Enter either 'y' or 'n'";
        }

        // Start the upgrade process
        QString upgradeCmd = WBIOServerCommon::getBotUpgradeCmd(client->botType);

        if (upgradeCmd.isEmpty()) {
            qDebug() << "CONSOLE:There is no upgrade script!";
            return;
        }

        // Get the software into the appropriate location
        QString swPath = WBIOServerCommon::getBotSoftwarePath(client->botType);
        if (swPath.isEmpty()) {
            qDebug() << "CONSOLE:Cannot find software path!";
            return;
        }

        // Location where new software is extracted to, before the upgrade
        QString tmpDestPath = QString(WBIO_CLIENT_BOTDIR_TMP_FORMAT)
                .arg(WBIO_DEFAULT_DBLOCATION)
                .arg(client->name)
                .arg(client->botType);

        // Create the directory for the integration software
        QDir destDir(tmpDestPath);
        if (!destDir.exists()) {
            if (!destDir.mkpath(tmpDestPath)) {
                qDebug() << "CONSOLE:Failed to create directory for integration bot software!";
                return;
            }
        }

        // First copy the software to the temporary location of the client's integration directory
        if (! integrationCopySW(client, swPath, tmpDestPath)) {
            return;
        }

        // Check if there is a VERSION file, if not use the version
        integrationUpdateVersionFile(tmpDestPath, newBotVerString);

        // Second perform the upgrade, by moving the software
        if (! integrationUpgrade(client, destPath, tmpDestPath)) {
            return;
        }

        // Third peform the installer if one does exist
        if (! integrationInstall(client, destPath)) {
            return;
        }

        // Lastly, peform the configure if one does exist
        QMap<QString,QString> clientValues;
        if (! integrationConfigure(client, destPath, clientValues)) {
            return;
        }
    }
}

bool
CmdClient::integrationCopySW(WickrBotClients *client, const QString& swPath, const QString& destPath)
{
    qDebug().noquote().nospace() << "CONSOLE:Copying " << client->botType << " software for " << client->user;
    {
        // Create a process to extract the software
        QProcess *unarchive = new QProcess(this);

        QString command = QString("tar -xf %1 -C %2").arg(swPath).arg(destPath);
        unarchive->setProcessChannelMode(QProcess::MergedChannels);
        unarchive->start(command, QIODevice::ReadWrite);

        // Wait for it to start
        if(!unarchive->waitForStarted()) {
            qDebug() << QString("CONSOLE:Failed to install %1 software!").arg(client->botType);
            return false;
        }

        // Continue reading the data until EOF reached
        QByteArray data;

        while(unarchive->waitForReadyRead())
            data.append(unarchive->readAll());

        // Output the data
        qDebug().noquote().nospace() << "CONSOLE:" << data;
    }
    return true;
}

void
CmdClient::integrationUpdateVersionFile(const QString& path, const QString& version)
{
    if (path.isEmpty() || version.isEmpty())
        return;

    QFile versFile(QString("%1/VERSION").arg(path));
    if (versFile.exists()) {
        QString  curBotVerString;

        unsigned curBotVer = getVersionNumber(&versFile);
        getIntegrationVersionString(curBotVer, curBotVerString);

        if (curBotVerString != version) {
            qDebug().noquote().nospace() << "CONSOLE:Warning: VERSION contains different value: " << curBotVerString << " expected " << version;
            return;
        }
    }

    // Create/update the version file
    versFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream out(&versFile);
    out << version << "\n";
    versFile.close();
}

bool
CmdClient::integrationInstall(WickrBotClients *client, const QString& destPath)
{
    // peform the installer if one does exist
    qDebug().noquote().nospace() << "CONSOLE:Installing " << client->botType << " software for " << client->user;
    {
        QStringList installOutput;
        QString installer = WBIOServerCommon::getBotInstaller(client->botType);
        if (!installer.isEmpty()){
            QString installerFullPath = QString("%1/%2").arg(destPath).arg(installer);

            QFile installShFile(installerFullPath);
            if (!installShFile.exists()) {
                qDebug() << "CONSOLE:install shell file does not exist for this software!";
                return false;
            }

            // Create a process to run the installer
            QProcess *runInstaller = new QProcess(this);
            runInstaller->setProcessChannelMode(QProcess::MergedChannels);
            runInstaller->setWorkingDirectory(destPath);
            runInstaller->start(installerFullPath, QIODevice::ReadWrite);

            // Wait for it to start
            if(!runInstaller->waitForStarted()) {
                qDebug() << QString("CONSOLE:Failed to run %1").arg(installer);
                return false;
            }

            while (runInstaller->state() != QProcess::NotRunning) {
                if (runInstaller->waitForReadyRead(2500)) {
                    QString bytes = QString(runInstaller->readAll());
                    //qDebug().noquote().nospace() << "CONSOLE:" << bytes;
                    installOutput.append(bytes);
                } else {
                    if (runInstaller->state() != QProcess::NotRunning) {
                        qDebug() << "CONSOLE:Installing";
                    }
                }
            }
        }
    }
    return true;
}

bool
CmdClient::integrationConfigure(WickrBotClients *client, const QString& destPath, const QMap<QString,QString>& keyValuePairs)
{
    qDebug().noquote().nospace() << "CONSOLE:Begin configuration of " << client->botType << " software for " << client->user;
    QString configure = WBIOServerCommon::getBotConfigure(client->botType);
    if (!configure.isEmpty()){
        QString configureFullpath = QString("%1/%2").arg(destPath).arg(configure);
        if (!runBotScript(destPath, configureFullpath, client, keyValuePairs)) {
            qDebug().noquote().nospace() << "CONSOLE:Failed to configure " << client->botType;
            return false;
        }
    }
    return true;
}

bool
CmdClient::integrationUpgrade(WickrBotClients *client, const QString& curSWPath, const QString& newSWPath)
{
    // peform the upgrade if one does exist
    qDebug().noquote().nospace() << "CONSOLE:Upgrading " << client->botType << " software for " << client->user;
    {
        QStringList upgradeOutput;
        QString upgrader = WBIOServerCommon::getBotUpgradeCmd(client->botType);
        if (!upgrader.isEmpty()){
            QString upgraderFullPath = QString("%1/%2").arg(newSWPath).arg(upgrader);

            QFile upgradeShFile(upgraderFullPath);
            if (upgradeShFile.exists()) {
                // Create a process to run the installer
                QProcess *runUpgrader = new QProcess(this);
                runUpgrader->setProcessChannelMode(QProcess::MergedChannels);
                runUpgrader->setWorkingDirectory(newSWPath);
                QStringList args;
                args.append(curSWPath);
                args.append(newSWPath);
                runUpgrader->start(upgraderFullPath, args, QIODevice::ReadWrite);

                // Wait for it to start
                if(!runUpgrader->waitForStarted()) {
                    qDebug() << QString("CONSOLE:Failed to run %1").arg(upgrader);
                    return false;
                }

                while(runUpgrader->waitForReadyRead()) {
                    QString bytes = QString(runUpgrader->readAll());
                    upgradeOutput.append(bytes);
                }
            }
            // If there is no upgrade shell then just move the directory
            else {
                qDebug() << "CONSOLE:There is no upgrade shell, moving the new software in.";
                QDir oldDir(curSWPath);
                if (oldDir.exists()) {
                    qDebug() << "CONSOLE:Removing old integration directory!";
                    if (!oldDir.removeRecursively()) {
                        qDebug() << "CONSOLE:Failed to delete the old integration directory!";
                        return false;
                    }
                }

                if (!oldDir.rename(newSWPath, curSWPath)) {
                    qDebug() << "CONSOLE:Failed to move new integration directory!";
                    return false;
                }
            }

        } else {
            return false;
        }
    }
    return true;
}


bool
CmdClient::configClients()
{
    bool    quit = false;
    QString temp;
    QString configFileName;

    // Get the name of the config file
    do {
        temp = getNewValue(configFileName, tr("Enter the config file name"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        // Check if the file exists
        QFile swFile(temp);
        if (swFile.exists()) {
            configFileName = temp;
            break;
        }

        qDebug() << "CONSOLE:Cannot find that file:" << temp;;
    } while (true);

    qDebug() << "CONSOLE:Processing" << configFileName;

    // Parse out the input ini file
    QSettings settings(configFileName, QSettings::IniFormat);
    QStringList clients_keys;
    settings.beginGroup(WIOCONFIG_CLIENTS_KEY);
    {
        // Only work on those that have a "true" value
        QStringList clients = settings.allKeys();
        for (QString clientKey : clients) {
            QString value = settings.value(clientKey, "false").toString();
            if (value == "true") {
                clients_keys.append(clientKey);
            }
        }
    }
    settings.endGroup();

    //TODO: check that the bot type is "wickrio_bot"
    QList<WBIOBotTypes *>integrations = WBIOServerCommon::getBotsSupported("wickrio_bot", false);


    QMap<QString,QMap<QString,QString>> clientsMap;
    bool errorsExist = false;

    for (QString clientName : clients_keys) {
        QMap<QString,QString> clientValues;
        bool skipClient=false;

        // Check if this client exists already
        for (WickrBotClients *client : m_clients) {
            if (clientName == client->user) {
                qDebug() << "CONSOLE:Username" << clientName << "already exists!\nCannot update a client from config file!";
                while (true) {
                    QString temp = getNewValue("no", tr("Do you want to skip this client?"), CHECK_BOOL);
                    if (temp.toLower() == "yes" || temp.toLower() == "y") {
                        qDebug() << "CONSOLE:Skipping client" << clientName;
                        break;
                    }
                    if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
                        qDebug() << "CONSOLE:Will not continue processing config file!";
                        return true;
                    }

                    // Check if the user wants to quit the action
                    if (handleQuit(temp, &quit) && quit) {
                        return false;
                    }
                }

                skipClient = true;
                break;
            }
        }
        // If the client exists and the user wants to skip then go to the next client
        if (skipClient)
            continue;

        settings.beginGroup(clientName);
        QStringList client_keys = settings.allKeys();
        for (QString key : client_keys) {
            QString value;

            // Check that the integration exists
            if (key == WIOCONFIG_INTEGRATION_KEY) {
                value = settings.value(key, "").toString();
                if (value.isEmpty()) {
                    continue;
                }

                bool foundIntegration=false;
                for (WBIOBotTypes *integration : integrations) {
                    if (integration->name() == value) {
                        foundIntegration = true;
                        break;
                    }
                }
                if (!foundIntegration) {
                    errorsExist = true;
                    qDebug().nospace() << "CONSOLE:For client " << clientName << " integration " << value << " does not exist!";
                }
            }
            // auto login will default to true, if it exists
            else if (key == WIOCONFIG_AUTO_LOGIN_KEY) {
                value = settings.value(key, "true").toString();
                if (value != "true" && value != "false") {
                    errorsExist = true;
                    qDebug().nospace() << "CONSOLE:For client " << clientName << " auto_login is " << value << " should be true or false!";
                }
            } else {
                value = settings.value(key, "").toString();
            }
            clientValues.insert(key, value);
        }
        settings.endGroup();

        // Default the autologin to true
        if (!clientValues.contains(WIOCONFIG_AUTO_LOGIN_KEY)) {
            clientValues.insert(WIOCONFIG_AUTO_LOGIN_KEY, "true");
        }

        clientsMap.insert(clientName, clientValues);
    }

    if (errorsExist) {
        qDebug() << "CONSOLE:Errors exist! Fix the errors and retry.";
        return true;
    }

    qDebug() << "CONSOLE:Begin adding clients!";

    QStringList clientNames = clientsMap.keys();
    for (QString clientName : clientNames) {
        qDebug() << "CONSOLE:Starting to create client" << clientName;

        // Get the map of values for this client
        QMap<QString,QString> clientMap = clientsMap.value(clientName);

        // Configure the WickrBotClients record needed to create the client
        WickrBotClients client;
        client.user = clientName;

        //TODO: Need to fix the way we determine the binary type
#if defined(WICKR_BETA)
        client.binary = "wickrio_botBeta";
#elif defined(WICKR_ALPHA)
        client.binary = "wickrio_botAlpha";
#elif defined(WICKR_PRODUCTION)
        client.binary = "wickrio_bot";
#else
        "Error no product type set!"
#endif

        // Process as many of the client values, othere will be passed to the create function
        QMap<QString,QString> configValues;
        for (QString key : clientMap.keys()) {
            QString value = clientMap.value(key);
            if (key == WIOCONFIG_AUTO_LOGIN_KEY) {
                client.m_autologin = (value == "true");
            } else if (key == WIOCONFIG_INTEGRATION_KEY) {
                client.botType = value;
            } else {
                configValues.insert(key, value);
            }
        }

        if (!getClientValues(&client, configValues, true)) {
            qDebug() << "CONSOLE:Failed to get client values and configure for" << clientName;
        } else {
            // Check that the record has already been added (likely during provisioning)
            WickrBotClients *existingClient = m_operation->m_ioDB->getClientUsingUserName(client.user);
            if (existingClient != nullptr) {
                client.id = existingClient->id;
            }

            // Add the new record to the database
            QString errorMsg = WickrIOConsoleClientHandler::addClient(m_operation->m_ioDB, &client);
            if (errorMsg.isEmpty()) {
                qDebug() << "CONSOLE:Successfully added client" << clientName;
            } else {
                qDebug() << "CONSOLE:Failed adding client" << clientName << "to the local database!";
                qDebug() << "CONSOLE:" << errorMsg;
            }
        }
    }

    // Update the list of clients
    m_clients = m_operation->m_ioDB->getClients();
}
