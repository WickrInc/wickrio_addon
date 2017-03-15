#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "consoleserverservice.h"
#include "wbio_common.h"
#include "wickriodatabase.h"

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
    m_isConfigured(false),
    m_settings(NULL)
{
    setServiceDescription(QObject::tr("WickrIO Console Server Service"));
    setServiceFlags(QtServiceBase::CanBeSuspended);

    m_appNm = WBIO_CONSOLESERVER_TARGET;

#if defined(Q_OS_LINUX)
#if defined(WICKR_TARGET_PROD)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio/plugins");
#elif defined(WICKR_TARGET_PREVIEW)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-prev/plugins");
#elif defined(WICKR_TARGET_BETA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-beta/plugins");
#elif defined(WICKR_TARGET_ALPHA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-alpha/plugins");
#else
    This is an issue, cannot set the library!
#endif
#endif

    m_operation = new OperationData();
    m_operation->processName = WBIO_CONSOLESERVER_TARGET;

    if (configureService()) {
        m_operation->updateProcessState(PROCSTATE_RUNNING);
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
    m_settings = WBIOCommon::getSettings();

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
//    m_operation->m_botDB = new WickrBotDatabase(m_operation->databaseDir);
    m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);


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

    if (!WBIOCommon::makeDirectory(dir.path())) {
        qDebug() << "WickrBot Server cannot make log directory:" << dir;
    } else {
        m_operation->setupLog(logFileName);
        m_operation->log("WickrIO Client Server configured");
    }

    m_operation->log("Database size", m_operation->m_botDB->size());

    m_isConfigured = true;
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
    if (m_isConfigured || configureService()) {
        m_operation->updateProcessState(PROCSTATE_RUNNING);
    }
}

/**
 * @brief WickrIOConsoleServerService::start
 * This is a service directive to start the service. This will start a timer that
 * will call the slotTimeoutProcess() slot on one second intervals, to perform the
 * tasks associated with this service.
 */
void WickrIOConsoleServerService::start()
{
    if (m_processTimer != NULL) {
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    /*
     * Configure and start the TCP listener
     * Need to create the cmdHandler first since it needs to read from the settings as well
     */
    CmdHandler *cmdHandler = new CmdHandler(m_settings, m_operation, this->parent());
    m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);

    // If the Console Server port is NOT set then exit
    if (m_settings->value(WBSETTINGS_CONSOLESVR_PORT, 0) == 0) {
        m_operation->updateProcessState(PROCSTATE_DOWN);
        qDebug() << "start: console server port NOT set";
        QCoreApplication::exit(0);
    }
    m_listener = new stefanfrings::HttpListener(m_settings, cmdHandler, this->parent());
    m_settings->endGroup();

    m_processTimer = new QTimer(0);
    connect(m_processTimer, &QTimer::timeout, this, &WickrIOConsoleServerService::slotTimeoutProcess);
    m_processTimer->setInterval(30000);
    m_processTimer->start();
}

/**
 * @brief WickrIOConsoleServerService::stop
 * This is the stop service directive to stop this service. This directive will send
 * the WBIO_IPCCMDS_STOP IPC command to all of the active clients.
 */
void WickrIOConsoleServerService::stop()
{
    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }

    // TODO: Stop the TCP listener
    m_listener->deleteLater();

    m_operation->updateProcessState(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOConsoleServerService::pause
 * This is the pause service directive to pause this service. A WBIO_IPCCMDS_STOP IPC
 * command will be sent to all of the clients.
 */
void WickrIOConsoleServerService::pause()
{
    if (m_processTimer != NULL) {
        if (m_processTimer->isActive()) {
            m_processTimer->stop();
        }
        m_processTimer->deleteLater();
        m_processTimer = NULL;
    }
}

/**
 * @brief WickrIOConsoleServerService::resume
 * This is the resume service directive to resume this service. The clients
 * will be restarted.
 */
void WickrIOConsoleServerService::resume()
{
    m_processTimer = new QTimer(0);
    QObject::connect(m_processTimer, &QTimer::timeout, this, &WickrIOConsoleServerService::slotTimeoutProcess);
    m_processTimer->setInterval(30000);
    m_processTimer->start();
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
