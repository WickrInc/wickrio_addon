#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include "cmdserver.h"
#include "wickrIOCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrioconsoleclienthandler.h"

CmdServer::CmdServer(CmdOperation *operation) :
    m_operation(operation)
{
    m_consoleServer = NULL;
}

/**
 * @brief CmdServer::runCommands
 * This function will handle user input associated with the Client Server commands.
 */
bool CmdServer::runCommands()
{
    if (m_consoleServer == NULL) {
        m_consoleServer = new ConsoleServer(m_operation->m_ioDB);
    }
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    while (true) {
        qDebug() << "CONSOLE:Enter server command:";
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();

            if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Client Server Commands:";
                qDebug() << "CONSOLE:  back      - leaves the client server setup";
                qDebug() << "CONSOLE:  help or ? - shows supported commands";
                qDebug() << "CONSOLE:  status    - shows the state of the clients server service";

                if (m_consoleServer->isRunning(WBIO_CLIENTSERVER_TARGET)) {
                    qDebug() << "CONSOLE:  stop     - stops the clients server service";
                } else {
                    qDebug() << "CONSOLE:  start    - starts the clients server service";
                }
                qDebug() << "CONSOLE:  quit       - leaves this program";
            } else if (cmd == "back") {
                break;
            } else if (cmd == "status") {
                status();
            } else if (cmd == "start") {
                m_consoleServer->toggleState(WBIO_CLIENTSERVER_TARGET);
            } else if (cmd == "stop") {
                m_consoleServer->toggleState(WBIO_CLIENTSERVER_TARGET);
            } else if (cmd == "quit") {
                return false;
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}

void CmdServer::status()
{
    QString clientState = WickrIOConsoleClientHandler::getActualProcessState(WBIO_CLIENTSERVER_TARGET, m_operation->m_ioDB);
    qDebug() << "CONSOLE:The Clients Server state is" << clientState;
}
