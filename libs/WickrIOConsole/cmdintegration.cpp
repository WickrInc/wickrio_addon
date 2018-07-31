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

bool CmdIntegration::processCommand(QStringList cmdList, bool &isquit)
{
    bool retVal = true;
    bool bForce = false;
    isquit = false;

    QString cmd = cmdList.at(0).toLower();
    int index;

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

    if (cmd == "?" || cmd == "help") {
        qDebug() << "CONSOLE:Commands:";
        qDebug() << "CONSOLE:  add         - create a new integration";
        qDebug() << "CONSOLE:  back        - leave the clients setup";
        qDebug() << "CONSOLE:  delete <#>  - deletes client with the specific index";
        qDebug() << "CONSOLE:  help or ?   - shows supported commands";
        qDebug() << "CONSOLE:  list        - shows a list of clients";
        qDebug() << "CONSOLE:  update <#>  - modifies a client with the specified index";
        qDebug() << "CONSOLE:  quit        - leaves this program";
    } else if (cmd == "add") {
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
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    // Get the data from the database
    bool isQuit;

    if (commands.isEmpty()) {
        while (true) {
            qDebug() << "CONSOLE:Enter integration command:";
            QString line = input.readLine();

            line = line.trimmed();
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
 * This funciton will print out a list of the current clients from the database.
 */
void CmdIntegration::listIntegrations()
{
    // get the list of custom integrations
    m_customInts = WBIOServerCommon::getBotsSupported("wickrio_bot", true);

    if (m_customInts.size() == 0) {
        qDebug() << "CONSOLE:There are no custom integrations currently configured!";
        return;
    }
    qDebug() << "CONSOLE:Current list of custom integrations:";

    QString version = "unknown";
    int cnt=0;
    for (WBIOBotTypes *customInt : m_customInts) {
        QString botSw = QString(WBIO_CUSTOMBOT_SWFILE).arg(customInt->name());
        QFile versionFile(QString(WBIO_CUSTOMBOT_VERSIONFILE).arg(customInt->name()));
        if (versionFile.exists()) {
            unsigned integrationVer = getVersionNumber(&versionFile);
            if (integrationVer > 0) {
                getVersionString(integrationVer, version);
            }
        }

        qDebug().noquote().nospace() << QString("CONSOLE: integration[%1] %2, version=%3")
                .arg(cnt++)
                .arg(customInt->name())
                .arg(version);
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
    m_customInts = WBIOServerCommon::getBotsSupported("wickrio_bot", true);

    for (WBIOBotTypes *customInt : m_customInts) {
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
bool CmdIntegration::addIntegration()
{
    bool quit = false;
    QString iName;
    QString iSwLocation;
    QString iVersion;
    QString temp;

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
        if (! botSwDir.mkdir( botSwDir.absolutePath())) {
            qDebug() << "CONSOLE:Cannot create directory for the new integration!";
            return true;
        }
    }

    QFile botSwFile(QString(WBIO_CUSTOMBOT_SWFILE).arg(iName));
    // if the file exists already, which it shouldn't, prompt the user
    if (botSwFile.exists()) {
        qDebug().nospace().noquote() << "CONSOLE:Software file exists already:" << botSwFile.fileName();
        bool deleteexisting = false;
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
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << iVersion << "\n";
        file.close();
    }

    // Add the custom integration
    WBIOServerCommon::addCustomIntegration(iName);
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

}

