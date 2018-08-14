#include <QTimer>

#include "cmdparser.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "consoleserver.h"
#include "wickrIOConsoleParserHandler.h"
#include "wickrIOIPCRuntime.h"

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
bool CmdParser::runCommands(QString commands)
{
    QTextStream input(stdin);

    // If the database location is not set then get it
    if (! m_operation->openDatabase())  {
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
    QStringList name;
    m_parsers = m_operation->m_ioDB->getParsers();
    QString currentState;
    int cnt=0;
    if(m_parsers.isEmpty()){
        qDebug() << "CONSOLE:No parsers running";
    }
    WickrBotProcessState* process;
    for (WickrIOParsers *parser : m_parsers) {
        QString processName = WBIOServerCommon::getParserProcessName(parser);
        process = new WickrBotProcessState;
        if(!m_operation->m_ioDB->getProcessState(processName, process)){
            qDebug() << "CONSOLE:Unable to find state of parser";
        }
        name = processName.split(".");
        currentState = WickrIOConsoleParserHandler::getActualProcessState(processName, processName, m_operation->m_ioDB);
        QString data = QString("CONSOLE:Index: %1   Name: %2   Id: %3   State: %4")
                .arg(cnt++)
                .arg(name[1])
                .arg(parser->id)
                .arg(currentState);
        delete process;
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
    m_parsers = m_operation->m_ioDB->getParsers();
    for (WickrIOParsers *parser : m_parsers) {
        if (parser->name == name) {
            qDebug() << "CONSOLE:The input name is NOT unique!";
            return true;
        }
    }
    return false;
}

/**
 * @brief CmdParser::chkParsersInterfaceExists
 * Check if the input interface and port combination is already used
 * by one of the parser records
 * @param name
 * @return
 */
bool CmdParser::chkParsersInterfaceExists(const QString& iface, int port)
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
    for (WickrBotClients *client : m_operation->m_ioDB->getClients()) {
        if (client->port == port && (client->iface == iface || client->iface == "localhost" || iface == "localhost" )) {
            qDebug() << "CONSOLE:The input interface and port are NOT unique!";
            return true;
        }
    }
    return false;
}

/**
 * @brief CmdParser::getParserValues
 * @param parser
 * Looks for ini file for parser type. If found, uses those values as a default. If not
 * sets some defaults. Checks with user about the values selected and assigns those values to
 * the parser pointer sent.
 * @return bool
 */
bool CmdParser::getParserValues(WickrIOParsers *parser)
{
    bool quit = false;
    QString temp;
    QString processName;
    QString currentName= "";


    // Determine the parser type
    QStringList possibleParserTypes = WBIOServerCommon::getAvailableParserApps();
    QString binary;
    if (possibleParserTypes.length() > 1) {
        temp = getNewValue(parser->binary, tr("Enter the bot type"), CHECK_LIST, possibleParserTypes);
        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        binary = temp;
    } else {
        binary = possibleParserTypes.at(0);
    }
    parser->binary = binary;

    processName = QString(WBIOServerCommon::getParserProcessName());
    QString configFileName =QString(WBIO_PARSER_SETTINGS_FORMAT).arg(m_operation->m_dbLocation).arg(parser->name);
    QFile iniFile(configFileName);
    if(iniFile.exists()){
        qDebug() << "CONSOLE:Config file already exists for this parser. Reading file to create default values";
        QSettings* parserSetting = new QSettings(configFileName, QSettings::NativeFormat);
        parserSetting->beginGroup(WBSETTINGS_PARSER_RABBIT);
        parser->user =      parserSetting->value(WBSETTINGS_PARSER_USER).toString();
        parser->password =  parserSetting->value(WBSETTINGS_PARSER_PASSWORD).toString();
        parser->port =      parserSetting->value(WBSETTINGS_PARSER_PORT).toInt();
        parser->host =      parserSetting->value(WBSETTINGS_PARSER_IF).toString();
        parser->queue =     parserSetting->value(WBSETTINGS_PARSER_QUEUE).toString();
        parser->exchange =  parserSetting->value(WBSETTINGS_PARSER_EXCHANGE).toString();
        parser->virtualhost=parserSetting->value(WBSETTINGS_PARSER_VIRTUAL_HOST).toString();
        parserSetting->endGroup();

        parserSetting->beginGroup(WBSETTINGS_PARSER_HEADER);
        parser->duration =  parserSetting->value(WBSETTINGS_PARSER_DURATION).toInt();
        temp =              parserSetting->value(WBSETTINGS_PARSER_NAME).toString();
        parserSetting->endGroup();
        parserSetting->deleteLater();
        parser->name = QString("%1.%2").arg(processName).arg(temp);
    }
    else {  
        //sets defaults that user can change
        parser->user = "admin";
        parser->port = 5672;
        parser->queue = "bot-messages";
        parser->exchange = "bot-messages";
        parser->virtualhost = "stats";
        parser->duration = 0;
    }

    // If more than one parser type, user selects desired parser
    QStringList binaries = WBIOServerCommon::getAvailableParserApps();
    if (binaries.length() == 1) {
        parser->binary = binaries.at(0);
    } else {
        while (true) {
            QString selected;
            selected = getNewValue(parser->binary, tr("Enter the binary"));
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
                    parser->binary = binary;
                    break;
                }
            }

            qDebug() << "CONSOLE:Invalid binary, enter one of" << binaries;
            continue;
        }
    }

    //Get unique Name for Parser
    if( parser->name != ""){
        QStringList nameParts = parser->name.split(".");
        currentName = nameParts[1];
    }
    do{
        temp = getNewValue(currentName, tr("Enter unique name for parser"));

        if(handleQuit(temp, &quit) && quit){
            return false;
        }
        while(temp.contains(".") || temp.contains(" ")){
            if(handleQuit(temp, &quit) && quit){
                return false;
            }
            if( temp.contains(".")){
                qDebug() << "CONSOLE:Name cannot contain \".\"";
            }
            else {
                qDebug() << "CONSOLE:Name cannot contain spaces";
            }
            temp = getNewValue(currentName, tr("Enter unique name for parser"));
        }
        temp = QString("%1.%2").arg(processName).arg(temp);
        if(! parser->name.isEmpty() && parser->name == temp)
            break;
    } while(chkParsersNameExists(temp));
    parser->name = temp;



    //Get User for Parser
    while(true){
        temp = getNewValue(parser->user, tr("Enter user"));
        if(handleQuit(temp, &quit) && quit){
            return false;
        }
        if(temp != NULL)
            break;
    }
    parser->user = temp;

    //Get password
    while (true){
        temp = getNewValue(parser->password, tr("Enter a password"));
        if(handleQuit(temp, & quit) && quit){
            return false;
        }
        if(temp.isEmpty() || temp.length() < 4){
            qDebug() << "CONSOLE:Password should be at lease 4 characters long!";
        } else {
            break;
        }
    }
    parser->password = temp;

    //Get duration from user
    while (true){
        temp = getNewValue(QString::number(parser->duration), tr("Enter a duration (must be a number)"),CHECK_INT);
        if(handleQuit(temp, & quit) && quit){
            return false;
        }
        if(temp.isEmpty()){
            qDebug() << "CONSOLE:Duration should not be null!";
        } else {
            break;
        }
    }
    int duration = temp.toInt();
    parser->duration= duration;


    // Get the list of possible interfaces
    QStringList ifaceList = WickrIOConsoleParserHandler::getNetworkInterfaceList();

    // Get a unique interface and port pair
    while (true) {
        temp = getNewValue(parser->host, tr("Enter the interface (list to see possible interfaces)"));

        // Check if the user wants to quit the action
        if (handleQuit(temp, &quit) && quit) {
            return false;
        }
        if (temp.toLower() == "list" || temp == "?") {
            foreach (QString iface, ifaceList) {
                qDebug() << "CONSOLE:" << iface;
            }
            continue;
        }
        if (!ifaceList.contains(temp)) {
            qDebug() << "CONSOLE:" << temp << "not found in the list of interfaces!";
            continue;
        }

        QString port = getNewValue(QString::number(parser->port), tr("Enter the IP port number"), CHECK_INT);

        // Check if the user wants to quit the action
        if (handleQuit(port, &quit) && quit) {
            return false;
        }

        int portinput = port.toInt();

        // Check if used the same values, if so continue on to the next field
        if (!port.isEmpty() && (parser->host == temp && parser->port == portinput)) {
            break;
        }

        if (chkParsersInterfaceExists(temp, portinput)) {
            qDebug() << "CONSOLE:Interface and port combination used already!";
            continue;
        }
        parser->host = temp;
        parser->port = portinput;
        break;
    }



    while(true){
        temp = getNewValue(parser->queue, "Enter the queue name of the parser");

        if( handleQuit(temp, &quit) && quit){
            return false;
        }

        if (temp.toLower() == "list" || temp == "?"){
            qDebug() << "CONSOLE:bot-messages";
            continue;
        }
        break;
    }
    parser->queue = temp;


    while(true){
        temp = getNewValue(parser->exchange, "Enter the exchange of the parser");

        if( handleQuit(temp, &quit) && quit){
            return false;
        }

        if (temp.toLower() == "list" || temp == "?"){
            qDebug() << "CONSOLE:bot-messages";
            continue;
        }
        break;
    }
    parser->exchange = temp;

    while(true){
        temp = getNewValue(parser->virtualhost, "Enter the virtual host of the parser");

        if( handleQuit(temp, &quit) && quit){
            return false;
        }

        if (temp.toLower() == "list" || temp == "?"){
            qDebug() << "CONSOLE:stats";
            continue;
        }
        break;
    }
    parser->virtualhost = temp;

    return !quit;
}


/**
 * @brief CmdParser::addParser
 * This function will add a new parser. The user will be prompted for the
 * appropriate fields for the new user. If the parser does not exist, it
 * will create a record in the database for it. Otherwise will update the ini
 * file.
 */
void CmdParser::addParser()
{
    QString errorMsg;
    WickrIOParsers *parser = new WickrIOParsers();
    WickrBotProcessState process;
    QString response;
    bool quit = false;
    while (true) {
        if (!getParserValues(parser)) {
            break;
        }
        //Convert parser to process state and insert into db
        if(m_operation->m_ioDB->getProcessState(parser->name, &process)){
            if(process.state == PROCSTATE_PAUSED){
                errorMsg = WickrIOConsoleParserHandler::modifyParser(m_operation->m_ioDB, parser);
                if(errorMsg.isEmpty()){
                    qDebug() << "CONSOLE:Record updated";
                    break;
                } else {
                qDebug() << "CONSOLE:"<< errorMsg;
                }
             }
            else {
                response = getNewValue("", tr("Parser not in paused state. Return to parser menu?"));
                if(response.toLower() == "yes" || response.toLower() == "y")
                    break;
                else if(handleQuit(response, & quit) && quit){
                    delete parser;
                    return;
                }
            }
        }
        else {
            errorMsg = WickrIOConsoleParserHandler::addParser(m_operation->m_ioDB, parser);
             if (errorMsg.isEmpty()) {
                qDebug() << "CONSOLE:Successfully added parser!";
                break;
            } else {
                qDebug() << "CONSOLE:" << errorMsg;
                // If the record was not added to the database then ask the user to try again
                response = getNewValue("", tr("Failed to add record, try again?"));
                if (response.isEmpty() || response.toLower() == "n") {
                    delete parser;
                    return;
                }
            }
        }
    }
    if(parser != NULL){
        delete parser;
    }
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
 * state first before it can be deleted otherwise unpredictable behavior as parser process may
 * restart despite ini file being deleted. If run again will resolve this.
 * @param parserIndex
 */
void CmdParser::deleteParser(int parserIndex)
{
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser = m_parsers.at(parserIndex);

        // Make sure the user wants to continue
        QString prompt = QString(tr("Do you really want to remove the parser with the name %1?")).arg(parser->name);
        QString response = getNewValue("", prompt);
        if (response.toLower() == "n" || response.toLower() == "no") {
            return;
        }

        WickrBotProcessState state;
        QString processName = WBIOServerCommon::getParserProcessName(parser);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:The parser is running, you should first Pause the parser, then delete!";
                QString response = getNewValue("", "Do you want to continue? (y or n)");
                if (response.toLower() == "n") {
                    return;
                 }
            }

            qDebug() << "CONSOLE:Deleting parser" << parser->name;
            if (! m_operation->m_ioDB->deleteProcessState(m_parsers.at(parserIndex)->name)) {
                qDebug() << "CONSOLE:There was a problem deleting the parser's process record!";
            }
            else {
                //Removing configuration file
                QString configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT).arg(m_operation->m_ioDB->m_dbDir).arg(processName);
                QFile configFile(configFileName);

                if(configFile.exists()){
                    configFile.remove();
                }
            }
            qDebug() << "CONSOLE:Process deleted";
            // Update the list of parsers
            m_parsers = m_operation->m_ioDB->getParsers();
        }
    }
}

/**
 * @brief CmdParser::modifyParser
 * This function will modify an existing parser's ini file.
 * @param parserIndex
 */
void CmdParser::modifyParser(int parserIndex)
{
    QString oldName;
    QString errorMsg;
    QString configFileName;
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser;
        WickrBotProcessState state;
        parser = m_parsers.at(parserIndex);
        oldName = parser->name;
        QString processName = WBIOServerCommon::getParserProcessName(parser);
        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                qDebug() << "CONSOLE:Cannot modify a running parser!";
            } else {
                while (true) {
                    if (!getParserValues(parser)) {
                        break;
                    }
                    //if name changes need to delete old record and create new database record and remove old ini file(ini made later in process)
                    if(oldName != parser->name){
                        m_operation->m_ioDB->deleteProcessState(oldName);
                        m_operation->m_ioDB->insertProcessState(parser->name, parser->id, PROCSTATE_DOWN);
                        configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT).arg(m_operation->m_ioDB->m_dbDir).arg(oldName);
                        QFile file(configFileName);
                        if(file.exists()){
                            file.remove();
                        }
                    }
                    // update the new record to the database
                    errorMsg = WickrIOConsoleParserHandler::modifyParser(m_operation->m_ioDB, parser);
                    if (errorMsg.isEmpty()) {
                        qDebug() << "CONSOLE:Successfully updated parser's parameters!";
                        qDebug() << "CONSOLE:Changes will take effect when parser started";
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
                }
            }
            m_parsers = m_operation->m_ioDB->getParsers();
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
bool CmdParser::sendParserCmd(const QString& dest, const QString& cmd)
{
    m_parserMsgInProcess = true;
    m_parserMsgSuccess = false;

    WickrIOIPCService *ipcSvc = WickrIOIPCRuntime::ipcSvc();

    if (ipcSvc == nullptr || ! ipcSvc->sendMessage(dest, false, cmd)) {
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
    WickrBotProcessState state;
    if (validateIndex(parserIndex)) {
        WickrIOParsers *parser = m_parsers.at(parserIndex);
        QString processName = WBIOServerCommon::getParserProcessName(parser);

        if (m_operation->m_ioDB->getProcessState(processName, &state)) {
            if (state.state == PROCSTATE_RUNNING) {
                QString prompt = QString(tr("Do you really want to pause the parser with the name %1?")).arg(parser->name);
                QString response = getNewValue("", prompt);
                if (response.toLower() == "y" || response.toLower() == "yes") {
                    if (! sendParserCmd(parser->name, WBIO_IPCCMDS_PAUSE)) {
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
                QString prompt = QString(tr("Do you really want to start the parser with the name %1?")).arg(parser->name);
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
            } else if(state.state == PROCSTATE_RUNNING){
                qDebug() << "CONSOLE:Parser must be in paused state to start it!";
            }
        } else {
            qDebug() << "CONSOLE:Could not get the parsers state!";
        }
    }
}

