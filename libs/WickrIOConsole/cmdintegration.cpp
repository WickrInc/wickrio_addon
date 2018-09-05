#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdintegration.h"

#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdIntegration::CmdIntegration(CmdOperation *operation) :
    m_operation(operation)
{
}

void
CmdIntegration::processHelp(const QStringList& cmdList)
{
    if (cmdList.length() > 0) {
        for (QString cmd : cmdList) {
            if (cmd == "add") {
                qDebug().nospace() << "CONSOLE:Add command usage: add\n"
                        << "  The add command is used to add a custom WickrIO bot integration. Before you\n"
                        << "  start to add a custom integration you should read the detailed documentation\n"
                        << "  which identifies all of the requirements. Before you add a custom WickrIO bot\n"
                        << "  integration you will need the following:\n\n"
                        << "  1. A name for the integration. The name should use characters, number, and\n"
                        << "     underscores.\n"
                        << "  2. Optionally a version number, which is made up of three numbers separated\n"
                        << "     by '.' character\n"
                        << "  3. A software.tar.gz file that contains the custom integration software. The\n"
                        << "     following executable scripts should also be contained in this file:\n"
                        << "     - install.sh   : run when installing the custom integration for a WickrIO\n"
                        << "                      bot client.\n"
                        << "     - configure.sh : run after the installation, to configure the custom\n"
                        << "                      integration. The WickrIO console program will parse the\n"
                        << "                      output from the configure.sh script so the user can be\n"
                        << "                      prompted to enter configuration values. Any time the\n"
                        << "                      configure.sh requires input from the user, the output\n"
                        << "                      from the configure.sh script should be started with the\n"
                        << "                      \"prompt:\" string. The user will be prompted to enter a\n"
                        << "                      value and that value will be sent to the configure.sh\n"
                        << "                      input stream. All other lines, without the \"prompt:\\n"
                        << "                      string will be displayed to the user.\n"
                        << "     - upgrade.sh   : run when the upgrade command is performed on the WickrIO\n"
                        << "                      bot client. This is necessary to save and restore\n"
                        << "                      information that was created during the running and/or\n"
                        << "                      configuration of the custom integration.\n"
                        << "     - start.sh     : run when the WickrIO bot client is started. Normally the\n"
                        << "                      custom integration will connect to the WickrIO bot client.\n"
                        << "     - stop.sh      : run when the WickrIO bot client is being stopped.\n\n"
                        << "  The install.sh, start.sh and stop.sh files are required. The configure.sh\n"
                        << "  and upgrade.sh files are optional, but are needed if you are setting values\n"
                        << "  needed to run the custom integration.\n";
            } else if (cmd == "back") {
                qDebug().nospace() << "CONSOLE:Back command usage: back\n"
                        << "  The back command will take you to the previous level of commands.\n";
            } else if (cmd == "delete") {
                qDebug().nospace() << "CONSOLE:Delete command usage: delete <integration number>\n"
                        << "  The delete command is used to delete an existing custom integration. The\n"
                        << "  <integration number> is the number shown on the 'list' command associated with\n"
                        << "  the WickrIO bot custom integration that you want to delete. The information\n"
                        << "  associated with the integration will be removed from the system.\n"
                        << "  NOTE: Any WickrIO bot clients that are using the deleted integration will not\n"
                        << "  be modified. You will have to go to each of those clients and perform a modify\n"
                        << "  command.\n";
            } else if (cmd == "help") {
                qDebug().nospace() << "CONSOLE:Help command usage: help [command ...]\n"
                        << "  Use the help command to display the list of commands you can use at this\n"
                        << "  command level. You can also supply a list of one or more commands on the same\n"
                        << "  line as the help command which the help command will display detailed\n"
                        << "  information for.\n";
            } else if (cmd == "list") {
                qDebug().nospace() << "CONSOLE:List command usage: list\n"
                        << "  The list command will display the list of WickrIO bot integrations that are\n"
                        << "  currently configured. The output will contain the list of integrations that\n"
                        << "  are supplied with the WickrIO software, and a list of the custom WickrIO bot\n"
                        << "  integrations. The following is a sample of what the output will look like:\n\n"
                        << "    List of included integrations:\n"
                        << "      compliance_bot\n"
                        << "      hubot, version=1.3.5\n"
                        << "      file_bot\n"
                        << "      welcome_bot\n"
                        << "    Current list of custom integrations:\n"
                        << "      [0] test_bot, version=3.4.5\n"
                        << "      [1] ncc_bot\n\n"
                        << "  The list of custom integrations has a number which you will need to use when\n"
                        << "  you perform any integration commands that affect a custom integration.\n";
            } else if (cmd == "quit") {
                qDebug().nospace() << "CONSOLE:Quit command usage: quit\n"
                        << "  The quit command is used to exit this program.\n"
                        << "  WARNING: leaving this program will also put all of the WickrrIO bot clients\n"
                        << "  into the paused state.\n";
            } else if (cmd == "update") {
                qDebug().nospace() << "CONSOLE:Update command usage: update <integration number>\n"
                        << "  The update command is used to update the software associated with a specific\n"
                        << "  WickrIO bot integration. The <integration numer> is the number shown on the\n"
                        << "  'list' command associated with the WickrIO bot integration that you want to\n"
                        << "  update. You will be prompted to enter the directory location of the\n"
                        << "  integration software file (software.tar.gz). The path you enter is the\n"
                        << "  directory that contains the file, it does not include the \"software.tar.gz\"\n"
                        << "  string. Optionally, you can also enter a version number to associate with this\n"
                        << "  new version.\n";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    } else {
        qDebug() << "CONSOLE:Commands:";
        qDebug() << "CONSOLE:  add         - create a new integration";
        qDebug() << "CONSOLE:  back        - go back to the previous command level";
        qDebug() << "CONSOLE:  delete <#>  - deletes integration with the specific index";
        qDebug() << "CONSOLE:  help or ?   - shows supported commands";
        qDebug() << "CONSOLE:  list        - shows a list of integrations";
        qDebug() << "CONSOLE:  update <#>  - modifies a integration with the specified index";
        qDebug() << "CONSOLE:  quit        - leaves this program";
    }
}

bool CmdIntegration::processCommand(QStringList cmdList, bool &isquit)
{
    bool retVal = true;
    bool bForce = false;
    isquit = false;

    QString cmd = cmdList.at(0).toLower();
    int index;

    if (cmd == "?" || cmd == "help") {
        QStringList args;
        if (cmdList.size() > 1) {
            args = cmdList;
            args.removeAt(0);
        }
        processHelp(args);
        return true;
    }

    // Convert the second argument to an integer, for the client index commands
    if (cmdList.size() > 1) {
        bool ok;
        index = cmdList.at(1).toInt(&ok);
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
        index = -1;
    }

    if (cmd == "add") {
        retVal = addIntegration();
    } else if (cmd == "back") {
        retVal = false;
    } else if (cmd == "delete") {
        if (index == -1) {
            qDebug() << "CONSOLE:Usage: delete <index>";
        } else {
            deleteIntegration(index);
        }
    } else if (cmd == "list") {
        listIntegrations();
    } else if (cmd == "update") {
        if (index == -1) {
            qDebug() << "CONSOLE:Usage: update <index>";
        } else {
            updateIntegration(index);
        }
    } else if (cmd == "quit") {
        retVal = false;
        isquit = true;
    } else {
        qDebug() << "CONSOLE:" << cmd << "is not a known command!";
    }
    return retVal;
}

/**
 * @brief CmdIntegration::runCommands
 * This function handles the setup of the different clients running on the system.
 * There can be multiple clients on a system. The user add new clients or modify
 * the configuration of existing clients.  Only clients in the paused state can be
 * modified. The user can start or pause a client as well from this function.
 */
bool CmdIntegration::runCommands(QString commands)
{
    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    // Get the data from the database
    bool isQuit;

    if (commands.isEmpty()) {
        while (true) {
            QString line = getCommand("Enter integration command:");
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

void CmdIntegration::status()
{
    listIntegrations();
}

unsigned
CmdIntegration::getVersionNumber(QFile *versionFile)
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
CmdIntegration::getVersionString(unsigned versionNum, QString& versionString)
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

bool
CmdIntegration::validateVersion(const QString& version)
{
    unsigned retVal = 0;
    QStringList slist = version.split(".");
    if (slist.length() == 3) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000 + slist.at(2).toInt();
    } else if (slist.length() == 2) {
        retVal = slist.at(0).toInt() * 1000000 + slist.at(1).toInt() * 1000;
    } else {
        return false;
    }
    return (retVal != 0);
}

/**
 * @brief CmdIntegration::listClients
 * This funciton will print out a list of the current integrations from the database.
 */
void CmdIntegration::listIntegrations()
{
    // get the list of all integrations
     QList<WBIOBotTypes *> allInts = WBIOServerCommon::getBotsSupported("wickrio_bot", false);

     qDebug() << "CONSOLE:List of included integrations:";
     for (WBIOBotTypes *curInt : allInts) {
         if (curInt->customBot())
             continue;
         QFile versionFile(QString(WBIO_CUSTOMBOT_VERSIONFILE).arg(curInt->name()));
         QString version;
         if (versionFile.exists()) {
             unsigned integrationVer = getVersionNumber(&versionFile);
             if (integrationVer > 0) {
                 getVersionString(integrationVer, version);
             }
         }

         if (version.isEmpty()) {
             qDebug().noquote().nospace() << QString("CONSOLE:  %1")
                     .arg(curInt->name());
         } else {
             qDebug().noquote().nospace() << QString("CONSOLE:  %1, version=%3")
                     .arg(curInt->name())
                     .arg(version);
         }
     }

    // get the list of custom integrations
    m_customInts = WBIOServerCommon::getBotsSupported("wickrio_bot", true);

    if (m_customInts.size() == 0) {
        qDebug() << "CONSOLE:There are no custom integrations currently configured!";
        return;
    }
    qDebug() << "CONSOLE:Current list of custom integrations:";

    int cnt=0;
    for (WBIOBotTypes *customInt : m_customInts) {
        QFile versionFile(QString(WBIO_CUSTOMBOT_VERSIONFILE).arg(customInt->name()));
        QString version;
        if (versionFile.exists()) {
            unsigned integrationVer = getVersionNumber(&versionFile);
            if (integrationVer > 0) {
                getVersionString(integrationVer, version);
            }
        }

        if (version.isEmpty()) {
            qDebug().noquote().nospace() << QString("CONSOLE:  [%1] %2")
                    .arg(cnt++)
                    .arg(customInt->name());
        } else {
            qDebug().noquote().nospace() << QString("CONSOLE:  [%1] %2, version=%3")
                    .arg(cnt++)
                    .arg(customInt->name())
                    .arg(version);
        }
    }
}

/**
  * @brief CmdIntegration::chkIntegrationExists
  * Check if the input name is the name of an existing custom interation
  * @param name
  * @return true if the integration exists already
  */
 bool
 CmdIntegration::chkIntegrationExists(const QString& name)
 {
    // get the list of custom integrations
    QList<WBIOBotTypes *> customInts = WBIOServerCommon::getBotsSupported("wickrio_bot", false);

    for (WBIOBotTypes *customInt : customInts) {
        if (customInt->name() == name)
            return true;
    }
    return false;
 }

/**
 * @brief CmdIntegration::addIntegration
 * This function will prompt the user for information about a new integration.
 * This new integration will be imported and placed in the appropriate location.
 */
bool CmdIntegration::addIntegration(const QString& updateName)
{
    bool quit = false;
    QString iName;
    QString iSwLocation;
    QString iVersion;
    QString temp;

    if (updateName.isEmpty()) {
        // Get a unique integration
        do {
            temp = getNewValue(iName, tr("Enter the integration name"));

            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return false;
            }
            if (!chkIntegrationExists(temp)) {
                break;
            }
            qDebug() << "CONSOLE:That name is already used as in integration!";
        } while (true);
        iName = temp;
    } else {
        iName = updateName;
    }

    // Get the location of the software.tar.gz fileA
    qDebug() <<"CONSOLE:Integration software must be contained in a software.tar.gz file.";
    do {
        temp = getNewValue(iSwLocation, tr("Enter the location of the software.tar.gz file"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        // Check if the file exists in that location
        QFile swFile(QString("%1/software.tar.gz").arg(temp));
        if (swFile.exists())
            break;

        qDebug() << "CONSOLE:Cannot find the software.tar.gz file in that location!";
    } while (true);
    iSwLocation = temp;

    // Do you want to enter a version
    bool getVersion = false;
    while (true) {
        QString temp = getNewValue("no", tr("Do you want to supply a version number?"), CHECK_BOOL);
        if (temp.toLower() == "yes" || temp.toLower() == "y") {
            getVersion = true;
            break;
        }
        if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
            getVersion = false;
            break;
        }

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
    }

    if (getVersion) {
        // Get the version number
        do {
            temp = getNewValue(iVersion, tr("Enter the version number (X.Y.Z format)"));

            // Check if the user wants to quit the action
            if (handleQuit(temp, &quit) && quit) {
                return false;
            }
            // Check if the file exists in that location
            if (validateVersion(temp))
                break;

            qDebug() << "CONSOLE:Version number is invalid!";
        } while (true);
        iVersion = temp;
    }


    QDir botSwDir(QString(WBIO_CUSTOMBOT_SWDIR).arg(iName));
    if (!botSwDir.exists()) {
        if (! botSwDir.mkpath(botSwDir.absolutePath())) {
            qDebug() << "CONSOLE:Cannot create directory for the new integration!";
            return true;
        }
    }

    QFile botSwFile(QString(WBIO_CUSTOMBOT_SWFILE).arg(iName));
    // if the file exists already, which it shouldn't, prompt the user
    if (botSwFile.exists()) {
        bool deleteexisting = false;

        if (updateName.isEmpty()) {
            qDebug().nospace().noquote() << "CONSOLE:Software file exists already:" << botSwFile.fileName();
            while (true) {
                QString temp = getNewValue("yes", tr("Do you want to delete the existing file?"), CHECK_BOOL);
                if (temp.toLower() == "yes" || temp.toLower() == "y") {
                    deleteexisting = true;
                    break;
                }
                if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
                    deleteexisting = false;
                    break;
                }

                // Check if the user wants to quit the action
                if (handleQuit(temp, &quit) && quit) {
                    return false;
                }
            }
        } else {
            deleteexisting = true;
        }

        if (deleteexisting) {
            if (! botSwFile.remove()) {
                qDebug() << "CONSOLE:Failed to remove the software file!";
                return true;
            }
        } else {
            qDebug() << "CONSOLE:Cannot continue!";
            return true;
        }
    }

    // Copy the software file to the custom integration's directory
    QString srcSwFile = QString("%1/software.tar.gz").arg(iSwLocation);
    QString dstSwFile = QString(WBIO_CUSTOMBOT_SWFILE).arg(iName);
    if (! QFile::copy(srcSwFile, dstSwFile)) {
        qDebug() << "CONSOLE:Failed to copy the software file!";
        return true;
    }

    // If the version has been set then create a VERSION file
    if (!iVersion.isEmpty()) {
        // Create a new file
        QFile file(QString(WBIO_CUSTOMBOT_VERSIONFILE).arg(iName));
        file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        QTextStream out(&file);
        out << iVersion << "\n";
        file.close();
    }

    // If we are adding (not update) then add the custom integration
    if (updateName.isEmpty()) {
        // Add the custom integration
        WBIOServerCommon::addCustomIntegration(iName);
    }
    return true;
}


/**
 * @brief CmdIntegration::validateIndex
 * Validate the input index. If invalid output a message
 * @param index
 */
bool CmdIntegration::validateIndex(int index)
{
    m_customInts = WBIOServerCommon::getBotsSupported("wickrio_bot", true);

    if (index >= m_customInts.length() || index < 0) {
        qDebug() << "CONSOLE:The input integrations index is out of range!";
        return false;
    }
    return true;
}

/**
 * @brief CmdIntegration::deleteIntegration
 * This function is used to delete an interation. The integration will be removed
 * from all clients that currently use it.
 *
 * TODO: Need to make sure that no clients are using this integration
 * @param index
 */
void CmdIntegration::deleteIntegration(int index)
{
    if (! validateIndex(index))
        return;

    WBIOBotTypes *integration = m_customInts.at(index);

    // Verify that there are NO clients using this integration
    QStringList clientsUsing;
    QList<WickrBotClients *> clients = m_operation->m_ioDB->getClients();
    for (WickrBotClients *client : clients) {
        if (client->botType == integration->name()) {
            clientsUsing.append(client->user);
        }
    }
    if (clientsUsing.length() > 0) {
        qDebug().nospace().noquote() << "CONSOLE:WARNING: There are clients using this integration:" << clientsUsing.join(',');
        qDebug().nospace().noquote() << "CONSOLE:WARNING: The software will not be removed from these clients!\n";
    }

    qDebug().nospace().noquote() << "CONSOLE:Deleting integration: " << integration->name();
    qDebug().noquote().nospace() << "CONSOLE:All software and version information will be lost!";

    // Make sure the user wants to delete the integration
    bool deleteexisting = false;
    while (true) {
        QString temp = getNewValue("yes", tr("Are you sure you want to delete this integration?"), CHECK_BOOL);
        if (temp.toLower() == "yes" || temp.toLower() == "y") {
            deleteexisting = true;
            break;
        }
        if (temp.toLower() == "no" || temp.toLower() == "n" || temp.isEmpty()) {
            return;
        }
    }

    QDir intDir(QString(WBIO_CUSTOMBOT_SWDIR).arg(integration->name()));
    if (! intDir.removeRecursively()) {
        qDebug() <<"CONSOLE:Failed to delete the directory!";
    } else {
        WBIOServerCommon::updateIntegrations();
    }
}

/**
 * @brief CmdIntegration::updateIntegration
 * This function will update the software for the specified integration.
 * @param index
 */
void CmdIntegration::updateIntegration(int index)
{
    if (! validateIndex(index))
        return;

    WBIOBotTypes *integration = m_customInts.at(index);

    // Verify that there are NO clients using this integration
    QStringList clientsUsing;
    QList<WickrBotClients *> clients = m_operation->m_ioDB->getClients();
    for (WickrBotClients *client : clients) {
        if (client->botType == integration->name()) {
            clientsUsing.append(client->user);
        }
    }
    if (clientsUsing.length() > 0) {
        qDebug().nospace().noquote() << "CONSOLE:WARNING: There are clients using this integration:" << clientsUsing.join(',');
        qDebug().nospace().noquote() << "CONSOLE:WARNING: The software will not be automatically updated for these clients!\n";
    }

    addIntegration(integration->name());
}

