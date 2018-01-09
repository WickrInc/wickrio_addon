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

    while (true) {
        qDebug() << "CONSOLE:Enter group [client, advanced, server, console or users]:";
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
            } else if (cmd == "users") {
                if (!m_cmdUsers.runCommands())
                    break;
            } else if (cmd == "quit") {
                qDebug() << "CONSOLE:Good bye!";
                break;
            } else if (cmd == "?") {
                qDebug() << "CONSOLE:Enter one of:";
                qDebug() << "CONSOLE:  client   - to setup the clients";
                qDebug() << "CONSOLE:  advanced - to setup the advanced settings";
                qDebug() << "CONSOLE:  server   - to setup the clients server settings";
                qDebug() << "CONSOLE:  console  - to setup the console server settings";
                qDebug() << "CONSOLE:  users    - to setup the mother bot users";
                qDebug() << "CONSOLE:  quit     - to exit the program";
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}
