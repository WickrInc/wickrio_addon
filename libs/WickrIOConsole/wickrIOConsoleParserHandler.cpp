#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QString>
#include <QNetworkInterface>

#include "wickrIOConsoleParserHandler.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotsettings.h"
#include "wickrbotprocessstate.h"
#include "wickrbotutils.h"


/**
 * @brief Parser::addParser
 * This function will add a new parser to the database. The input WickrBotProcessState class
 * contains all of the necessary information to add the new parser.  All of the necessary
 * database and settings files will be created. The new parser will be created in the
 * paused state
 * @param parser pointer to the WickrBotProcessState class for the new parser
 * @return an error string is returned if an error occurs during processing. If no error then
 * returns empty string
 */
QString
WickrIOConsoleParserHandler::addParser(WickrIOClientDatabase *ioDB, WickrIOParsers *parser)
{
    QString errorSettings;
    errorSettings = handleSettings(ioDB, parser);
    if(errorSettings != ""){
        return errorSettings;
    }

    //test that application can start
    if(ioDB->insertProcessState(parser->name, parser->id, PROCSTATE_DOWN)){
        qDebug() << "CONSOLE:Successfully added record to database!";

    } else {
        qDebug() << "CONSOLE:Unable to insert into database!";
        return QCoreApplication::tr("Could not insert into database");
    }


    // Set the state of the client process to paused
    if (! ioDB->updateProcessState(parser->name, 0, PROCSTATE_PAUSED)) {
        return QCoreApplication::tr("Could not create process state record!");
    }
    return QString("");
}


/**
 * @brief Parser::handleSettings(WickrIOClientDatabase *ioDB, WickrIOParsers *parser)
 * Writes ini file with the settings given to the WickrIOParsers sent. Returns string with
 * error message from any error encountered
 */
QString
WickrIOConsoleParserHandler::handleSettings(WickrIOClientDatabase *ioDB, WickrIOParsers *parser)
{
    QString logname;
    QString configFileName;
    QStringList parserNames;
    QString parserType;
    QString parserName;

    // Create the configuration file
    QString dbDir(ioDB->m_dbDir);
    // If the db is open the proceed, otherwise return false
    if (! ioDB->isOpen()) {
        return QCoreApplication::tr("Database is not open!");
    }

#ifdef Q_OS_WIN
    configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT)
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET)
            .arg(parser->m_name);
#else
    configFileName = QString(WBIO_PARSER_SETTINGS_FORMAT).arg(ioDB->m_dbDir).arg(parser->name);
#endif

    logname = QString(WBIO_PARSER_LOGFILE_FORMAT).arg(ioDB->m_dbDir).arg(parser->name);

    qDebug() << "**********************************";
    qDebug() << "Creating ini file for " << parser->name;
    qDebug() << "dbDir=" << dbDir;
    qDebug() << "logname=" << logname;
    qDebug() << "Config File=" << configFileName;
    qDebug() << "**********************************";

    parserNames = parser->name.split(".");
    if (parserNames.length() > 1){
        parserType = parserNames[0];
        parserName = parserNames[1];

    }
    // The client DB directory MUST exist already
    if (! QDir(ioDB->m_dbDir).exists()) {
        return QString("%1 does not exist").arg(ioDB->m_dbDir);
    }

    QFile file(configFileName);

    if (file.exists()) {
        file.remove();
    }
    QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

    settings->beginGroup(WBSETTINGS_PARSER_RABBIT);
    //currently all the hosts set to same as assuming localhost. Need to know what values of hosts should be otherwise
    settings->setValue(WBSETTINGS_PARSER_ALPHA_HOST, parser->host);
    settings->setValue(WBSETTINGS_PARSER_BETA_HOST, parser->host);
    settings->setValue(WBSETTINGS_PARSER_IF, parser->host);
    settings->setValue(WBSETTINGS_PARSER_BETA, parser->host);

    settings->setValue(WBSETTINGS_PARSER_PORT,parser->port);
    settings->setValue(WBSETTINGS_PARSER_USER, parser->user);
    settings->setValue(WBSETTINGS_PARSER_PASSWORD, parser->password);
    settings->setValue(WBSETTINGS_PARSER_OLD_PASSWORD, parser->password);
    settings->setValue(WBSETTINGS_PARSER_EXCHANGE, parser->exchange);
    settings->setValue(WBSETTINGS_PARSER_QUEUE, parser->queue);
    settings->setValue(WBSETTINGS_PARSER_VIRTUAL_HOST, parser->virtualhost);

    settings->endGroup();


    settings->beginGroup(WBSETTINGS_PARSER_HEADER);
    settings->setValue(WBSETTINGS_PARSER_LOGFILE, logname);
    settings->setValue(WBSETTINGS_PARSER_DATABASE, ioDB->m_dbDir);
    settings->setValue(WBSETTINGS_PARSER_DURATION, parser->duration);
    settings->setValue(WBSETTINGS_PARSER_NAME, parserName);
    settings->endGroup();
    settings->sync();

    return QString("");
}
/**
 * @brief Parser::modifyParser
 * This function will change the ini file of a parser. The input WickrBotProcessState class
 * contains all of the necessary information to change ini file.
 * @param newParser pointer to the WickrBotProcessState class for the new parser
 * @return an error string is returned if an error occurs during processing. If no error then
 * the returned string is empty
 */
QString
WickrIOConsoleParserHandler::modifyParser(WickrIOClientDatabase *ioDB, WickrIOParsers *parser)
{
    QString errorSettings;
    errorSettings = handleSettings(ioDB, parser);
    return errorSettings;
}


/**
 * @brief WickrIOConsoleParserHandler::getNetworkInterfaceList
 * This function will return a list of the network interfaces that can be
 * used to accept WickrIO network traffic. The first entry will always be the
 * "localhost" entry.
 * @return
 */
QStringList WickrIOConsoleParserHandler::getNetworkInterfaceList()
{
    QStringList interfaceList;
    interfaceList.append(QString("localhost"));

#if 0
    foreach (QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            interfaceList.append(iface.name());
        }
    }
#else
    QNetworkInterface interface;
    QList<QHostAddress> IpList = interface.allAddresses();

    foreach (QHostAddress addr, IpList) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress(QHostAddress::LocalHost)) {
            interfaceList.append(addr.toString());
        }
    }
#endif
    return interfaceList;
}



/**
 * @brief WickrIOConsoleClientHandler::getActualProcessState
 * This function will get the process state from the WickrIO database. If the state is NOT
 * down or paused it will also verify that the process is running.
 * TODO: Should the process be updated in the database if it is found to not be running?
 * @param processName
 * @param ioDB
 * @return
 */
QString
WickrIOConsoleParserHandler::getActualProcessState(const QString &processName, WickrIOClientDatabase* ioDB, int timeout)
{
    WickrBotProcessState state;
    QString parserState = "UNKNOWN";

    if (ioDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_DOWN) {
            parserState = "Down";
        } else if (state.state == PROCSTATE_RUNNING) {
            parserState = "Running";

            const QDateTime dt = QDateTime::currentDateTime();
            int secs = state.last_update.secsTo(dt);

            // If greater than the timeout then check if the process is running
            if (secs > timeout) {
#if 0
                QString appName = QFileInfo(QCoreApplication::arguments().at(0)).fileName();
#else
                QString appName = "WickrIO";
#endif
                if (WickrBotUtils::isRunning(appName, state.process_id)) {
//                    WickrBotUtils::killProcess(state.process_id);
                    parserState = "Hung?";
                } else {
                    parserState = "Not Running";
                }
            }

        } else if (state.state == PROCSTATE_PAUSED) {
            parserState = "Paused";
        }
    }

    return parserState;
}
