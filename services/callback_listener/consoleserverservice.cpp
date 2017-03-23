#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotcore.h>
#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "consoleserverservice.h"
#include "wbio_common.h"

extern bool isVERSIONDEBUG();

/**
 * @brief WickrIOConsoleServerService::WickrIOConsoleServerService
 * This is the constructor for the WickrIO service (windows) / Daemon (linux)
 * @param argc
 * @param argv
 */
WickrIOConsoleServerService::WickrIOConsoleServerService(int argc, char **argv) :
    QtService<QCoreApplication>(argc, argv, WBIO_CONSOLESERVER_TARGET),
    m_operation(NULL),
    m_processTimer(NULL),
    m_vendorName(QString(WBIO_ORGANIZATION)),
    m_configFileName(""),
    m_isConfigured(false),
    m_settings(NULL)
{
    setServiceDescription(QObject::tr("WickrBotIO Client Server Service"));
    setServiceFlags(QtServiceBase::CanBeSuspended);

    m_appNm = WBIO_CONSOLESERVER_TARGET;

#if defined(Q_OS_LINUX)
    if( isVERSIONDEBUG() ) {
        QCoreApplication::addLibraryPath("/usr/lib/wickrio-beta/plugins");
    } else {
        QCoreApplication::addLibraryPath("/usr/lib/wickrio/plugins");
    }
#endif

    m_operation = new OperationData();
    m_operation->processName = WBIO_CONSOLESERVER_TARGET;

    // ONLY supports specifying the config/settings file location
    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd.startsWith("-config=") ) {
            m_configFileName = cmd.remove("-config=");
        }
    }

    /*
     * Find the configuration file
     * TODO: Need OS specific setting for this. Linux=.ini, OSX=.plist, Win=registry
     */
    if (m_configFileName.isEmpty()) {
        m_configFileName = searchConfigFile();
    }

    // Get the process ID of this process
    m_operation->pid = QCoreApplication::applicationPid();

    if (configureService()) {
        updateProcessState(PROCSTATE_RUNNING);
    }
}

WickrIOConsoleServerService::~WickrIOConsoleServerService()
{
}

/**
 * @brief WickrIOConsoleServerService::configureService
 * This function will configure the setup of this service.  Specifically the
 * database file location, log file location, output file location will be
 * setup.
 * @return True is returned if the service is configured, false if not
 */
bool WickrIOConsoleServerService::configureService()
{
    qDebug() << "Entering configService()";

    if (m_configFileName.isEmpty()) {
        return false;
    }

    qDebug() << "ConfigFile=" << m_configFileName;

    m_settings=new QSettings(m_configFileName, QSettings::NativeFormat, this);

    QString logFileName("");
    m_operation->databaseDir = "";

    /*
     * Make sure the database directory is set
     */
    m_settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    m_operation->databaseDir = m_settings->value(WBSETTINGS_DATABASE_DIRNAME).toString();
    m_settings->endGroup();

    // If there is NO database directory set then return failed!
    if (m_operation->databaseDir.isEmpty()) {
        qDebug() << "Leaving configService() failed!";
        m_settings->deleteLater();
        m_settings = NULL;
        return false;
    }

    // If there is a database directory then create the Wickrbot database
    m_operation->m_botDB = new WickrBotDatabase(m_operation->databaseDir);


    /*
     * Setup the logs file
     */
    m_settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    logFileName = m_settings->value(WBSETTINGS_LOGGING_FILENAME).toString();
    m_settings->endGroup();

    // If a logfile name is NOT set then put a log file into the database directory
    if (logFileName.isEmpty()) {
        logFileName = m_operation->databaseDir + "/logs/" + m_operation->processName + ".log";
    }

    // Check that can create the log file
    QFileInfo fileInfo(logFileName);
    QDir dir;
    dir = fileInfo.dir();

    if (!makeDirectory(dir.path())) {
        qDebug() << "WickrBot Server cannot make log directory:" << dir;
    } else {
        m_operation->setupLog(logFileName);
        m_operation->log("WickrBotIO Client Server configured");
    }

    m_operation->log("Database size", m_operation->m_botDB->size());

    m_isConfigured = true;
    qDebug() << "Leaving configService() successful";
    return true;
}

/**
 * @brief WickrIOConsoleServerService::slotTimeoutProcess
 * This function is called when the service timer expires, which is every
 * second. This is only when the service has been started.  When called this
 * function will only perform actions if the service has been configured. For
 * Windows this means the registry has been setup, for Linux this requires the
 * settings INI file to be setup. If everything is configured then the list of
 * client process' will be checked and if necessary the clients will be started.
 */
void WickrIOConsoleServerService::slotTimeoutProcess()
{
    qDebug() << "Entering slotTimeoutProcess";
    if (m_isConfigured || configureService()) {
        if (! --m_statusCntDwn) {
            updateProcessState(PROCSTATE_RUNNING);
        }
    }
    qDebug() << "Leaving slotTimeoutProcess";
}

/**
 * @brief WickrIOConsoleServerService::start
 * This is a service directive to start the service. This will start a timer that
 * will call the slotTimeoutProcess() slot on one second intervals, to perform the
 * tasks associated with this service.
 */
void WickrIOConsoleServerService::start()
{
    qDebug() << "begin start()";

    m_statusCntDwn = UPDATE_STATUS_SECS;

    if (m_processTimer != NULL) {
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    m_processTimer = new QTimer(0);
    connect(m_processTimer, &QTimer::timeout, this, &WickrIOConsoleServerService::slotTimeoutProcess);
    m_processTimer->setInterval(30000);
    m_processTimer->start();

    /*
     * Configure and start the TCP listener
     * Need to create the cmdHandler first since it needs to read from the settings as well
     */
    CmdHandler *cmdHandler = new CmdHandler(m_settings, m_operation, this->parent());
    m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    m_listener = new HttpListener(m_settings, cmdHandler, this->parent());
    m_settings->endGroup();

    qDebug() << "finish start()";
}

/**
 * @brief WickrIOConsoleServerService::stop
 * This is the stop service directive to stop this service. This directive will send
 * the WBIO_IPCCMDS_STOP IPC command to all of the active clients.
 */
void WickrIOConsoleServerService::stop()
{
    qDebug() << "begin stop()";

    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    // TODO: Stop the TCP listener
    m_listener->deleteLater();

    updateProcessState(PROCSTATE_DOWN);

    qDebug() << "finish stop()";
}

/**
 * @brief WickrIOConsoleServerService::pause
 * This is the pause service directive to pause this service. A WBIO_IPCCMDS_STOP IPC
 * command will be sent to all of the clients.
 */
void WickrIOConsoleServerService::pause()
{
    qDebug() << "begin pause()";

    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    qDebug() << "finish pause()";
}

/**
 * @brief WickrIOConsoleServerService::resume
 * This is the resume service directive to resume this service. The clients
 * will be restarted.
 */
void WickrIOConsoleServerService::resume()
{
   qDebug() << "begin resume()";

    m_processTimer = new QTimer(0);
    QObject::connect(m_processTimer, &QTimer::timeout, this, &WickrIOConsoleServerService::slotTimeoutProcess);
    m_processTimer->setInterval(30000);
    m_processTimer->start();

    qDebug() << "finish resume()";
}

/**
 * @brief WickrIOConsoleServerService::processCommand
 * @param code
 */
void WickrIOConsoleServerService::processCommand(int code)
{
    QString cmd = QString("received processCommand = %1").arg(code);
    qDebug() << "in processCommand:" << cmd;
}



bool WickrIOConsoleServerService::makeDirectory(QString dirname)
{
    QDir tmp(dirname);
    if (!tmp.exists()) {
        if (!tmp.mkpath(".")) {
            qDebug() << "Cannot make directory" << dirname;
            return false;
        }
    }
    return true;
}

void WickrIOConsoleServerService::updateProcessState(int state)
{
    m_operation->log("Update process state to =", state);
    m_operation->m_botDB->updateProcessState(m_operation->processName, m_operation->pid, state);
}

/**
 * @brief searchConfigFile
 * Search for the configuration file. Depends on the operating system.
 * TODO: Put this in a library for all of the tools to use
 * @return
 */
QString WickrIOConsoleServerService::searchConfigFile() {
#ifdef Q_OS_WIN
    return QString(WBIO_SERVER_SETTINGS_FORMAT).arg(m_vendorName).arg(WBIO_GENERAL_TARGET);
#else
    // Setup the list of locations to search for the ini file
    QString filesdir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );

    QString fileName = QString("%1.ini").arg(WBIO_CLIENTSERVER_TARGET);

    QStringList searchList;
    searchList.append(filesdir);
    QString optDir = QString("/opt/%1").arg(WBIO_GENERAL_TARGET);
    searchList.append(optDir);

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    // If not found then try the WickrBotServer INI file
    if (retFile.isEmpty()) {
        // not found
        foreach (QString dir, searchList) {
            qWarning("%s/%s not found",qPrintable(dir),qPrintable(fileName));
        }
        qFatal("Cannot find config file %s",qPrintable(fileName));
        return 0;
    }
    return retFile;
#endif
}

