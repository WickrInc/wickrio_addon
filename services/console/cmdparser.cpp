#include "cmdparser.h"

CmdParser::CmdParser(CmdOperation *operation) :
    m_operation(operation)
{
}

/**
 * @brief CmdParser::runCommands
 * This function handles the setup of the different parsers running on the system.
 * There can be multiple parsers on a system. The user add new parsers or modify
 * the configuration of existing parsers.  Only parsers in the paused state can be
 * modified. The user can start or pause a parser as well from this function.
 */
bool CmdParser::runCommands()
{
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_operation->openDatabase()) {
        qDebug() << "CONSOLE:Cannot open database!";
        return true;
    }

    // Get the data from the database
    m_parsers = m_operation->m_ioDB->getParsers();

    while (true) {
        qDebug() << "CONSOLE:Enter parser command:";
        QString line = input.readLine();

        line = line.trimmed();
        if (line.length() > 0) {
            QStringList args = line.split(" ");
            QString cmd = args.at(0).toLower();
            int parserIndex;

            // Convert the second argument to an integer, for the parser index commands
            if (args.size() > 1) {
                bool ok;
                parserIndex = args.at(1).toInt(&ok);
                if (!ok) {
                    qDebug() << "CONSOLE:Parser Index is not a number!";
                    continue;
                }
            } else {
                parserIndex = -1;
            }

            if (cmd == "?" || cmd == "help") {
                qDebug() << "CONSOLE:Commands:";
                qDebug() << "CONSOLE:  add        - adds a new parser";
                qDebug() << "CONSOLE:  back       - leave the parsers setup";
                qDebug() << "CONSOLE:  delete <#> - delete a parser with the specific index";
                qDebug() << "CONSOLE:  help or ?  - shows supported commands";
                qDebug() << "CONSOLE:  list       - shows a list of parsers";
                qDebug() << "CONSOLE:  modify <#> - modifies a parser with the specified index";
                qDebug() << "CONSOLE:  pause <#>  - pauses the parser with the specified index";
                qDebug() << "CONSOLE:  start <#>  - starts the parser with the specified index";
                qDebug() << "CONSOLE:  quit       - leaves this program";
            } else if (cmd == "add") {
                addParser();
            } else if (cmd == "back") {
                break;
            } else if (cmd == "delete") {
                if (parserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: delete <index>";
                } else {
                    deleteParser(parserIndex);
                }
            } else if (cmd == "list") {
                listParsers();
            } else if (cmd == "modify") {
                if (parserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: modify <index>";
                } else {
                    modifyParser(parserIndex);
                }
            } else if (cmd == "pause") {
                if (parserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: pause <index>";
                } else {
                    pauseParser(parserIndex);
                }
            } else if (cmd == "quit") {
                return false;
            } else if (cmd == "start") {
                if (parserIndex == -1) {
                    qDebug() << "CONSOLE:Usage: start <index>";
                } else {
                    startParser(parserIndex);
                }
            } else {
                qDebug() << "CONSOLE:" << cmd << "is not a known command!";
            }
        }
    }
    return true;
}

void CmdParser::status()
{
    listParsers();
}

/**
 * @brief CmdParser::listParsers
 */
void CmdParser::listParsers()
{
    // Update the list of parsers
    m_parsers = m_operation->m_ioDB->getParsers();

    int cnt=0;
    for (WickrIOParsers *parser : m_parsers) {
        QString processName = WBIOServerCommon::getParserProcessName(parser);

        QString data = QString("CONSOLE: parser[%1] %2")
            .arg(cnt++)
            .arg(parser->m_id);
        qDebug() << qPrintable(data);
    }
}

/**
 * @brief CmdParser::chkParsersNameExists
 * Check if the input name is already used by one of the parser records
 * @param name
 * @return
 */
bool CmdParser::chkParsersNameExists(const QString& name)
{
    for (WickrIOParsers *parser : m_parsers) {
        if (parser->m_name == name) {
            qDebug() << "CONSOLE:The input name is NOT unique!";
            return true;
        }
    }
    return false;
}

bool CmdParser::getParserValues(WickrIOParsers *parser)
{
    bool quit = false;
    QString temp;

    // Get a unique parser name
    do {
        temp = getNewValue(parser->m_name, tr("Enter an unique Name for this parser (this is a local name, not the Wickr ID)"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }

        // Allow the user to re-use the same name if it was previously set
        if (! parser->m_name.isEmpty()  && parser->m_name == temp) {
            break;
        }
    } while (chkParsersNameExists(temp));
    parser->m_name = temp;

    // Determine the parser type
    QStringList possibleParserTypes = WBIOServerCommon::getAvailableParserApps();
    QString binary;
    if (possibleParserTypes.length() > 1) {
        temp = getNewValue(parser->m_binary, tr("Enter the bot type"), CHECK_LIST, possibleParserTypes);
        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        binary = temp;
    } else {
        binary = possibleParserTypes.at(0);
    }
    parser->m_binary = binary;

    // Time to configure the BOT's username

    // Determine if the BOT has an executable that will create/provision the user

    QString provisionApp = WBIOServerCommon::getProvisionApp(binary);

    if (provisionApp != nullptr) {
        QString parserDbDir;
        QString command = parser->m_binary;
        QStringList arguments;

        QString configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT).arg(m_operation->m_dbLocation).arg(parser->m_name);
        parserDbDir = QString("%1/clients/%2/client").arg(m_operation->m_dbLocation).arg(parser->m_name);

        arguments.append(QString("-config=%1").arg(configFileName));
        arguments.append(QString("-clientdbdir=%1").arg(parserDbDir));
        arguments.append(QString("-processname=%1").arg(WBIOServerCommon::getParserProcessName(parser)));

        m_exec = new QProcess();

        connect(m_exec, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotCmdFinished));
        connect(m_exec, SIGNAL(finished(int, QProcess::readyReadStandardOutput)), this, SLOT(slotCmdOutputRx));

        m_exec->start(command, arguments);

        if (m_exec->waitForStarted(-1)) {
            m_exec->waitForFinished(-1);
        } else {
            QByteArray errorout = m_exec->readAllStandardError();
            if (!errorout.isEmpty()) {
                qDebug() << "ERRORS" << errorout;
            }
            qDebug() << "Exit code=" << m_exec->exitCode();
        }
        m_exec->close();

    } else {
    }

    // Get the binary to use
    QStringList binaries = WBIOServerCommon::getAvailableParserApps();
    if (binaries.length() == 1) {
        parser->m_binary = binaries.at(0);
    } else {
        while (true) {
            QString selected;
            selected = getNewValue(parser->m_binary, tr("Enter the binary"));
            // Check if the user wants to quit the action
            if (handleQuit(selected, &quit) && quit) {
                return false;
            }

            if (selected.toLower() == "list" || selected == "?") {
                foreach (QString binary, binaries) {
                    qDebug() << "CONSOLE:" << binary;
                }
                continue;
            }

            for (QString binary : binaries) {
                if (selected == binary) {
                    parser->m_binary = binary;
                    break;
                }
            }

            qDebug() << "CONSOLE:Invalid binary, enter one of" << binaries;
            continue;
        }
    }

    return !quit;
}

void CmdParser::slotCmdFinished(int, QProcess::ExitStatus)
{
    qDebug() << "process completed";
    m_exec->deleteLater();
    m_exec = nullptr;
}

void CmdParser::slotCmdOutputRx()
{
    QByteArray output = m_exec->readAll();

    qDebug() << output;

    QTextStream s(stdin);
    QString lineInput = s.readLine();
    QByteArray input(lineInput.toUtf8());
    m_exec->write(input);
}


/**
 * @brief CmdParser::addParser
 * This function will add a new parser. The user will be prompted for the
 * appropriate fields for the new user.
 */
void CmdParser::addParser()
{
    WickrIOParsers *parser = new WickrIOParsers();

    while (true) {
        if (!getParserValues(parser)) {
            break;
        }

        // Add the new record to the database
#if 1
        qDebug() << "TODO: Implement addParser()";
        break;
#else
        QString errorMsg = WickrIOConsoleParserHandler::addParser(m_operation->m_ioDB, parser);
        if (errorMsg.isEmpty()) {
            qDebug() << "CONSOLE:Successfully added record to the database!";
            break;
        } else {
            qDebug() << "CONSOLE:" << errorMsg;
            // If the record was not added to the database then ask the user to try again
            QString response = getNewValue("", tr("Failed to add record, try again?"));
            if (response.isEmpty() || response.toLower() == "n") {
                delete parser;
                return;
            }
        }
#endif
    }
    delete parser;

    // Update the list of parsers
    m_parsers = m_operation->m_ioDB->getParsers();
}



/**
 * @brief CmdParser::validateIndex
 * Validate the input index. If invalid output a message
 * @param parserIndex
 */
bool CmdParser::validateIndex(int parserIndex)
{
    if (parserIndex >= m_parsers.length() || parserIndex < 0) {
        qDebug() << "CONSOLE:The input parser index is out of range!";
        return false;
    }
    return true;
}

/**
 * @brief CmdParser::deleteParser
 * This function is used to delete a parser.  The parser must be in the paused
 * state first before it can be deleted.
 * @param parserIndex
 */
void CmdParser::deleteParser(int parserIndex)
{
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser = m_parsers.at(parserIndex);

        // Make sure the user wants to continue
        QString prompt = QString(tr("Do you really want to remove the parser with the name %1")).arg(parser->m_name);
        QString response = getNewValue("", prompt);
        if (response.toLower() == "n") {
            return;
        }

        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getParserProcessName(parser);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.ipc_port == 0) {
                qDebug() << "CONSOLE:Parser does not have an IPC port defined, will not be able to stop WickrIO parser process!";
                QString response = getNewValue("", "Do you want to continue? (y or n)");
                if (response.toLower() == "n") {
                    return;
                }
            } else if (state.state == PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:The parser is running, you should first Pause the parser, then delete!";
                QString response = getNewValue("", "Do you want to continue? (y or n)");
                if (response.toLower() == "n") {
                    return;
                }
            }

            qDebug() << "CONSOLE:Deleting parser" << parser->m_name;

#if 1
            qDebug() << "TODO: implement deleteParser";
#else
            if (! m_operation->m_ioDB->deleteParserUsingName(m_parsers.at(parserIndex)->m_name)) {
                qDebug() << "CONSOLE:There was a problem deleting the parser!";
            }
#endif
            if (! m_operation->m_ioDB->deleteProcessState(processName)) {
                qDebug() << "CONSOLE:There was a problem deleting the parser's process record!";
            }

            // TODO: Need to cleanup the parsers directory and registry (for windows)

            // Update the list of parsers
            m_parsers = m_operation->m_ioDB->getParsers();
        }
    }
}

/**
 * @brief CmdParser::modifyParser
 * This function will modify an exist parser.
 * @param parserIndex
 */
void CmdParser::modifyParser(int parserIndex)
{
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser;
        WickrBotProcessState state;
        parser = m_parsers.at(parserIndex);

        QString processName = WBIOServerCommon::getParserProcessName(parser);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:Cannot modify a running parser!";
            } else {
                while (true) {
                    if (!getParserValues(parser)) {
                        break;
                    }

#if 1
                    qDebug() << "TODO: implement modifyParser";
                    break;
#else
                    // update the new record to the database
                    QString errorMsg = WickrIOConsoleParserHandler::addParser(m_operation->m_ioDB, parser);
                    if (errorMsg.isEmpty()) {
                        qDebug() << "CONSOLE:Successfully updated record to the database!";
                        break;
                    } else {
                        qDebug() << "CONSOLE:" << errorMsg;
                        // If the record was not updated to the database then ask the user to try again
                        QString response = getNewValue("", tr("Failed to update record, try again?"));
                        if (response.isEmpty() || response.toLower() == "n") {
                            delete parser;
                            return;
                        }
                    }
#endif
                }
                m_parsers = m_operation->m_ioDB->getParsers();
            }
        }
    }
}

/**
 * @brief CmdParser::sendParserCmd
 * This function will send and wait for a successful sent of a message to a
 * parser, with the input port number.
 * @param port
 * @param cmd
 * @return
 */
bool CmdParser::sendParserCmd(int port, const QString& cmd)
{
    m_parserMsgInProcess = true;
    m_parserMsgSuccess = false;

    if (! m_operation->m_ipc->sendMessage(port, cmd)) {
        return false;
    }

    QTimer timer;
    QEventLoop loop;

    loop.connect(m_operation->m_ipc, SIGNAL(signalSentMessage()), SLOT(quit()));
    loop.connect(m_operation->m_ipc, SIGNAL(signalSendError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 6;

    while (loopCount-- > 0) {
        timer.start(10000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "CONSOLE:Timed out waiting for stop parser message to send!";
        }
    }
    return true;
}

/**
 * @brief CmdParser::pauseParser
 * This function is used to pause a running parser
 * @param parserIndex
 */
void CmdParser::pauseParser(int parserIndex)
{
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser = m_parsers.at(parserIndex);
        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getParserProcessName(parser);

        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.ipc_port == 0) {
                qDebug() << "CONSOLE:Parser does not have an IPC port defined, cannot pause!";
            } else if (state.state == PROCSTATE_RUNNING) {
                QString prompt = QString(tr("Do you really want to pause the parser with the name %1")).arg(parser->m_name);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    if (! sendParserCmd(state.ipc_port, WBIO_IPCCMDS_PAUSE)) {
                        qDebug() << "CONSOLE:Failed to send message to parser!";
                    }
                }
            } else {
                qDebug() << "CONSOLE:parser must be running to pause it!";
            }
        } else {
            qDebug() << "CONSOLE:Could not get the parsers state!";
        }
    }
}

/**
 * @brief CmdParser::startParser
 * This function is used to start a stopped or paused parser
 * @param parserIndex
 */
void CmdParser::startParser(int parserIndex)
{
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser = m_parsers.at(parserIndex);
        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getParserProcessName(parser);

        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_PAUSED) {
                QString prompt = QString(tr("Do you really want to start the parser with the name %1")).arg(parser->m_name);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    if (! m_operation->m_ioDB->updateProcessState(processName, 0, PROCSTATE_DOWN)) {
                        qDebug() << "CONSOLE:Failed to change start of parser in database!";
                    }
                }
            } else if (state.state == PROCSTATE_DOWN){
                qDebug() << "CONSOLE:Parser is in Down state. The WickrIO parser Server should change the state to running.";
                qDebug() << "CONSOLE:If this is not happening, please check that the WickrIOSvr process is running!";
                //TODO: Check on the state of the WickrIO Server!
            } else {
                qDebug() << "CONSOLE:Parser must be in paused state to start it!";
            }
        } else {
            qDebug() << "CONSOLE:Could not get the parsers state!";
        }
    }
}

