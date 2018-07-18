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
#include "wickrioapi.h"

CmdMain::CmdMain(OperationData*pOperation) :
    m_cmdOperation(pOperation),
    m_cmdClient(&m_cmdOperation),
    m_cmdConsole(&m_cmdOperation),
    m_cmdAdvanced(&m_cmdOperation),
    m_cmdServer(&m_cmdOperation),
    m_cmdParser(&m_cmdOperation),
    m_cmdUsers(&m_cmdOperation)
{
}

bool CmdMain::processCommand(QString cmd, QString subcmds)
{
    bool retVal=true;

    // Handle the case for commands that accept options
    QStringList options = cmd.split(" ");
    QString command;
    if (options.size() > 0) {
        command = options.at(0);
        options.removeFirst();
    }

    if (command == "client") {
        retVal = m_cmdClient.runCommands(options, subcmds);
    } else if (cmd == "config") {
        retVal = config(subcmds);
    } else if (cmd == "advanced") {
        retVal = m_cmdAdvanced.runCommands(subcmds);
    } else if (cmd == "server") {
        retVal = m_cmdServer.runCommands(subcmds);
    } else if (cmd == "console") {
        retVal = m_cmdConsole.runCommands(subcmds);
    } else if (m_hasMotherBotBinary && cmd == "users") {
        retVal = m_cmdUsers.runCommands(subcmds);
    } else if (m_hasParserBinary && cmd == "parser") {
        retVal = m_cmdParser.runCommands(subcmds);
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

            // Get input from the user
            QString line = input.readLine();

            // Breakup the input; handle options and other possible commands
            line = line.trimmed();
            if (line.length() > 0) {
                QStringList arglist = line.split(",");
                QString cmd = arglist.at(0).toLower().trimmed();
                QString args;
                if (arglist.length() > 1)
                    args = arglist.at(1);

                if (! processCommand(cmd, args))
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

bool CmdMain::config(QString args)
{
    if (! QFile::exists(args)) {
        qDebug() << "CONSOLE:config file does not exist!";
        return true;
    }
    // Parse JSON file format
    if (args.endsWith(".json")) {
        QString val;
        QFile file;
        file.setFileName(args);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        val = file.readAll();
        file.close();

        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        QJsonObject sett2 = d.object();
        QJsonValue value;

        WickrIOSSLSettings ssl;
        bool sslSet = false;

        if (sett2.contains(APIJSON_CFG_SSL)) {

            value = sett2[APIJSON_CFG_SSL];
            QJsonObject sslObject = value.toObject();

            if (sslObject.contains(APIJSON_CFG_SSLKEY)) {
                QJsonValue idobj = sslObject[APIJSON_CFG_SSLKEY];
                ssl.sslKeyFile = idobj.toString();

                if (! ssl.validateSSLKey(ssl.sslKeyFile)) {
                    qDebug() << "CONSOLE:SSL Key filename is not valid!";
                    return true;
                }
            }
            if (sslObject.contains(APIJSON_CFG_SSLCERT)) {
                QJsonValue idobj = sslObject[APIJSON_CFG_SSLCERT];
                ssl.sslCertFile = idobj.toString();

                if (! ssl.validateSSLCert(ssl.sslCertFile)) {
                    qDebug() << "CONSOLE:SSL Key filename is not valid!";
                    return true;
                }
            }

            // Both files must be entered
            if (ssl.sslKeyFile.isEmpty() || ssl.sslCertFile.isEmpty()) {
                qDebug() << "CONSOLE:SSL configuration must contain both valid Key and Certificate files!";
                return true;
            }

            sslSet = true;
        }

        if (sett2.contains(APIJSON_CFG_CLIENTS)) {
            QJsonArray entryArray;

            value = sett2[APIJSON_CFG_CLIENTS];
            entryArray = value.toArray();
            for (int i=0; i< entryArray.size(); i++) {
                QJsonValue arrayValue = entryArray[i];

                if (arrayValue.isObject()) {
                    QJsonObject arrayObject = arrayValue.toObject();
                    QString name, password, ifacetype, apikey;
                    int port;

                    if (arrayObject.contains(APIJSON_CFG_NAME)) {
                        QJsonValue idobj = arrayObject[APIJSON_CFG_NAME];
                        name = idobj.toString();
                    }
                    if (arrayObject.contains(APIJSON_CFG_PASSWORD)) {
                        QJsonValue idobj = arrayObject[APIJSON_CFG_PASSWORD];
                        password = idobj.toString();
                    }
                    if (arrayObject.contains(APIJSON_CFG_IFACETYPE)) {
                        QJsonValue idobj = arrayObject[APIJSON_CFG_IFACETYPE];
                        ifacetype = idobj.toString();
                    }
                    if (arrayObject.contains(APIJSON_CFG_APIKEY)) {
                        QJsonValue idobj = arrayObject[APIJSON_CFG_APIKEY];
                        apikey = idobj.toString();
                    }
                    if (arrayObject.contains(APIJSON_CFG_PORT)) {
                        QJsonValue idobj = arrayObject[APIJSON_CFG_PORT];
                        port = idobj.toInt();
                    }
                    // Check that the minimum values have been entered
                    if (name.isEmpty()) {
                        qDebug() << "CONSOLE:You must enter at least the client name!";
                        return true;
                    }

                    WickrBotClients *client = new WickrBotClients();
                    client->name = name;
                    client->name.replace("@", "_");
                    client->user = name;

                    if (!password.isEmpty()) {
                        client->password = password;
                    }
                    if (!apikey.isEmpty()) {
                        client->apiKey = apikey;
                    }
                    if (!ifacetype.isEmpty()) {

                    }
                    if (port != 0) {

                    }

                    client->m_handleInbox = true;
                    client->botType = QString();
                }
            }
        }

        if (sslSet) {
            m_operation->m_settings->beginGroup(WBSETTINGS_SSL_HEADER);
            ssl.saveToSettings(m_operation->m_settings);
            m_operation->m_settings->endGroup();

            ConsoleServer consoleServer(m_operation->m_ioDB);
            if (consoleServer.setSSL(&ssl)) {
                qDebug() << "CONSOLE:The Console Server needs to restart for changes to take effect.";
                qDebug() << "CONSOLE:Restarting Console Server!";
                consoleServer.restart();
                qDebug() << "CONSOLE:Done restarting Console Server!";
            }
        }
    } else {
        qDebug() << "CONSOLE:config file format not supported! Only support JSON currently.";
    }
    return true;
}
