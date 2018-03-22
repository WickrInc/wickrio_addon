#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdmain.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleClientHandler.h"

CmdMain::CmdMain() :
    m_cmdOperation(nullptr),
    m_cmdClient(&m_cmdOperation),
    m_cmdConsole(&m_cmdOperation),
    m_cmdAdvanced(&m_cmdOperation),
    m_cmdServer(&m_cmdOperation),
    m_cmdParser(&m_cmdOperation),
    m_cmdUsers(&m_cmdOperation)
{
}

bool CmdMain::processCommand(QString cmd, QString args)
{
    bool retVal=true;

    if (cmd == "client") {
        retVal = m_cmdClient.runCommands(args);
    } else if (cmd == "advanced") {
        retVal = m_cmdAdvanced.runCommands(args);
    } else if (cmd == "server") {
        retVal = m_cmdServer.runCommands(args);
    } else if (cmd == "console") {
        retVal = m_cmdConsole.runCommands(args);
    } else if (m_hasMotherBotBinary && cmd == "users") {
        retVal = m_cmdUsers.runCommands(args);
    } else if (m_hasParserBinary && cmd == "parser") {
        retVal = m_cmdParser.runCommands(args);
    } else if (cmd == "quit") {
        qDebug() << "CONSOLE:Good bye!";
        retVal = false;
    } else if (cmd == "?") {
        qDebug() << "CONSOLE:Enter one of:";
        qDebug() << "CONSOLE:  client   - to setup the clients";
        qDebug() << "CONSOLE:  advanced - to setup the advanced settings";
        qDebug() << "CONSOLE:  server   - to setup the clients server settings";
        qDebug() << "CONSOLE:  console  - to setup the console server settings";
        if(m_hasParserBinary){
            qDebug() << "CONSOLE:  parser   - to setup the parser settings";
        }
        if (m_hasMotherBotBinary) {
            qDebug() << "CONSOLE:  users    - to setup the mother bot users";
        }
        qDebug() <<     "CONSOLE:  quit     - to exit the program";
    } else {
        qDebug() << "CONSOLE:" << cmd << "is not a known command!";
    }

    return retVal;
}

/**
 * @brief CmdMain::runCommands
 * This function handles the input commands for the top level comand input.
 * The user will have to select an area to traverse into or quit.
 */
bool CmdMain::runCommands(QString commands)
{
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_cmdOperation.openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return false;
    }

    // Check if there is a mother bot binary installed
    m_hasMotherBotBinary = (WBIOServerCommon::getAvailableMotherClients().length() > 0);
    m_hasParserBinary = (WBIOServerCommon::getAvailableParserApps().length() >0);

    if (commands.isEmpty()) {
        while (true) {
            if (m_hasMotherBotBinary) {
                if(m_hasParserBinary){
                    qDebug() << "CONSOLE:Enter one of [client, advanced, server, console, parser or users]:";
                } else {
                    qDebug() << "CONSOLE:Enter one of [client, advanced, server, console or users]:";
                }
            } else {
                if(m_hasParserBinary){
                    qDebug() << "CONSOLE:Enter one of [client, advanced, server, console or parser]:";
                } else {
                    qDebug() << "CONSOLE:Enter one of [client, advanced, server or console]:";
                }
            }

            QString line = input.readLine();

            line = line.trimmed();
            if (line.length() > 0) {
                QStringList args = line.split(" ");
                QString cmd = args.at(0).toLower();

                if (! processCommand(cmd, QString()))
                    break;
            }
        }
    } else {
        QStringList args = commands.split(",");
        QString cmd = args.at(0).toLower();

        args.removeFirst();
        QString arguments = args.join(',');

        processCommand(cmd, arguments);
    }
    return true;
}
