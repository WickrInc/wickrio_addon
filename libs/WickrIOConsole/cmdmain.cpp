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

/**
 * @brief CmdMain::runCommands
 * This function handles the input commands for the top level comand input.
 * The user will have to select an area to traverse into or quit.
 */
bool CmdMain::runCommands()
{
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_cmdOperation.openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return false;
    }

    // Check if there is a mother bot binary installed
    bool hasMotherBotBinary = (WBIOServerCommon::getAvailableMotherClients().length() > 0);
    bool hasParserBinary = (WBIOServerCommon::getAvailableParserApps().length() >0);
    while (true) {
        if (hasMotherBotBinary) {
            if(hasParserBinary){
                qDebug() << "CONSOLE:Enter group [client, advanced, server, console, parser or users]:";
            } else {
                qDebug() << "CONSOLE:Enter group [client, advanced, server, console or users]:";
            }
        } else {
            if(hasParserBinary){
                qDebug() << "CONSOLE:Enter group [client, advanced, server, console or parser]:";
            } else {
                qDebug() << "CONSOLE:Enter group [client, advanced, server or console]:";
            }
        }
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            if (cmd == "client") {
                if (!m_cmdClient.runCommands())
                    break;
            } else if (cmd == "advanced") {
                if (!m_cmdAdvanced.runCommands())
                    break;
            } else if (cmd == "server") {
                if (!m_cmdServer.runCommands())
                    break;
            } else if (cmd == "console") {
                if (!m_cmdConsole.runCommands())
                    break;
            } else if (hasParserBinary && cmd == "parser") {
                if (!m_cmdParser.runCommands())
                    break;
            } else if (hasMotherBotBinary && cmd == "users") {
                if (!m_cmdUsers.runCommands())
                    break;
            } else if (cmd == "quit") {
                qDebug() << "CONSOLE:Good bye!";
                break;
            } else if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Enter one of:";
                qDebug() << "CONSOLE:  client   - to setup the clients";
                qDebug() << "CONSOLE:  advanced - to setup the advanced settings";
                qDebug() << "CONSOLE:  server   - to setup the clients server settings";
                qDebug() << "CONSOLE:  console  - to setup the console server settings";
                if(hasParserBinary){
                    qDebug() << "CONSOLE:  parser   - to setup the parser settings";
                }

                if (hasMotherBotBinary) {
                    qDebug() << "CONSOLE:  users    - to setup the mother bot users";
                }
                qDebug() <<     "CONSOLE:  quit     - to exit the program";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}
