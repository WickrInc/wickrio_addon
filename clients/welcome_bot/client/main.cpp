#include "wickrIOCommon.h"
#include "wickrbotsettings.h"

#include <QDebug>
//#include <QPlainTextEdit>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "session/wickrSession.h"
#include "user/wickrApp.h"
#include "common/wickrRuntime.h"

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "welcomeClientConfigInfo.h"

#include "wickrIOClientRuntime.h"
#include "welcomeClientRxDetails.h"

#include "common/wickrUtil.h"

#ifdef WICKR_PLUGIN_SUPPORT
Q_IMPORT_PLUGIN(WickrPlugin)
#endif

#ifdef Q_OS_MAC
#include "platforms/mac/extras/WickrAppDelegate-C-Interface.h"
extern void wickr_powersetup(void);
#endif

#include <httpserver/httplistener.h>

#include "wickrIOClientMain.h"
#include "wickrIOIPCService.h"
#include "wickrbotutils.h"
#include "operationdata.h"

#include "requesthandler.h"

extern int versionForLogin();
extern QString getPlatform();
extern QString getVersionString();
extern QString getBuildString();
extern QString getAppVersion();

OperationData *operation = NULL;
RequestHandler *requestHandler = NULL;
stefanfrings::HttpListener *httpListener = NULL;

// TODO: UPDATE THIS
static void
usage()
{
    qDebug() << "Args are [-cmd|-gui(default)|-both][-noexclusive][-user=userid] [-script=scriptfile] [-crypt] [-nocrypt]";
    qDebug() << "If you specify a userid, the database will be named" << WickrDBAdapter::getDBName() + "." + "userid";
    qDebug() << "If you specify a script file, it will run to completion, then command line will run";
    qDebug() << "If you specify -noexclusive, the db will not be locked for exclusive open";
    qDebug() << "By default, in debug mode, the database will not be encrypted (-nocrypt)";
    exit(0);
}

/** Search the configuration file */
QString
searchConfigFile() {
#ifdef Q_OS_WIN
    return QString(WBIO_SERVER_SETTINGS_FORMAT)
            .arg(QCoreApplication::organizationName())
            .arg(QCoreApplication::applicationName());
#else
    // Setup the list of locations to search for the ini file
    QString filesdir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );

    QStringList searchList;
    searchList.append(filesdir);

    // Look for the ini file with the application name
    QString appName=QCoreApplication::applicationName();
    QString fileName(appName+".ini");

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    if (retFile.isEmpty()) {
        qFatal("Cannot find config file %s",qPrintable(fileName));
    }
    return retFile;
#endif
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

int main(int argc, char *argv[])
{
    QCoreApplication *app = NULL;

    Q_INIT_RESOURCE(welcome_bot);

    // Setup appropriate library values based on Beta or Production client
    QByteArray secureJson;
    bool isDebug;
    if (isVERSIONDEBUG()) {
        secureJson = "secex_json2:Fq3&M1[d^,2P";
        isDebug = true;
    } else {
        secureJson = "secex_json:8q$&M4[d^;2R";
        isDebug = false;
    }

    QString username;
    QString appname = WBIO_CLIENT_TARGET;
    QString orgname = WBIO_ORGANIZATION;

    wickrProductSetProductType(BOT_PRODUCT_TYPE);
    WickrURLs::setDefaultBaseURLs(ClientConfigurationInfo::DefaultBaseURL,
                                  ClientConfigurationInfo::DefaultDirSearchBaseURL);

    qDebug() <<  appname << "System was booted" << WickrUtil::formatTimestamp(WickrAppClock::getBootTime());

    bool dbEncrypt = true;

    operation = new OperationData();
    operation->processName = WBIO_CLIENT_TARGET;
    operation->m_cleanDBOnLogout = true;

    QString clientDbPath("");
    QString suffix;
    QString wbConfigFile("");
    bool setProcessName = false;
    bool setDuration = false;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if( cmd.startsWith("-dbdir=") ) {
            operation->databaseDir = cmd.remove("-dbdir=");
        } else if (cmd.startsWith("-log=") ) {
            QString logFile = cmd.remove("-log=");
            operation->log_handler->setupLog(logFile);
        } else if (cmd.startsWith("-clientdbdir=")) {
            clientDbPath = cmd.remove("-clientdbdir=");
        } else if (cmd.startsWith("-config=")) {
            wbConfigFile = cmd.remove("-config=");
        } else if (cmd.startsWith("-suffix")) {
            suffix = cmd.remove("-suffix=");
            WickrUtil::setTestAccountMode(suffix);
        } else if (cmd.startsWith("-force") ) {
            // Force the WickBot Client to run, regardless of the state in the database
            operation->force = true;
        } else if (cmd.startsWith("-rcv")) {
            QString temp = cmd.remove("-rcv=");
            if (temp.compare("on", Qt::CaseInsensitive) ||
                temp.compare("true", Qt::CaseInsensitive)) {
                operation->receiveOn =true;
            }
        } else if (cmd.startsWith("-processname")) {
            operation->processName = cmd.remove("-processname=");
            setProcessName = true;
        } else if (cmd.startsWith("-duration")) {
            QString temp = cmd.remove("-duration=");
            operation->duration = temp.toLong();
            setDuration = true;
        }
    }

    if( isVERSIONDEBUG() ) {
        for( int argidx = 1; argidx < argc; argidx++ ) {
            QString cmd(argv[argidx]);

            if( cmd == "-?" || cmd == "-help" || cmd == "--help" )
                usage();

            if( cmd == "-crypt" ) {
                dbEncrypt = true;
            }
            else if( cmd == "-nocrypt" ) {
                dbEncrypt = false;
            }
            else if( cmd.startsWith("-user=") ) {
                username = cmd.remove("-user=");
                WickrUtil::setTestAccountMode(username);
            }
            else if( cmd == "-noexclusive" ) {
                WickrDBAdapter::setDatabaseExclusiveOpenStatus(false);
            }
        }
    }

#ifdef Q_OS_MAC
    WickrAppDelegateInitialize();
    //wickrAppDelegateRegisterNotifications();
    //QString pushid = wickrAppDelegateGetNotificationID();
#endif

    app = new QCoreApplication(argc, argv);

    qDebug() << QApplication::libraryPaths();
    qDebug() << "QLibraryInfo::location(QLibraryInfo::TranslationsPath)" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    qDebug() << "QLocale::system().name()" << QLocale::system().name();

    qDebug() << "app " << appname << " for " << orgname;
    QCoreApplication::setApplicationName(appname);
    QCoreApplication::setOrganizationName(orgname);

#ifdef WICKR_PRODUCTION
    qDebug().nospace().noquote() << "CONSOLE:" << "Version: " << ClientVersionInfo::getVersionString();
#else
    qDebug().nospace().noquote() << "CONSOLE:" << "Version: " << ClientVersionInfo::getVersionString() <<
                                    " " << ClientVersionInfo::getBuildString();
#endif

    // Production mode
    bool productionMode;
#ifdef WICKR_PRODUCTION
    productionMode = true;
#else
    productionMode = false;
#endif

    // Client type
    WickrCore::WickrRuntime::WickrClientType clientType;
#if defined(WICKR_MESSENGER)
    clientType = WickrCore::WickrRuntime::MESSENGER;
#elif defined(WICKR_ENTERPRISE)
    clientType = WickrCore::WickrRuntime::ENTERPRISE;
#else
    clientType = WickrCore::WickrRuntime::PROFESSIONAL;
#endif

    // Set the path where the Device ID will be set
    WickrUtil::botDeviceDir = clientDbPath;

    // Wickr Runtime Environment (all applications include this line)
    WickrCore::WickrRuntime::init(argc, argv,
                                  BOT_PRODUCT_TYPE,
                                  ClientVersionInfo::getOrgName(),
                                  appname,
                                  ClientConfigurationInfo::DefaultBaseURL,
                                  ClientConfigurationInfo::DefaultDirSearchBaseURL,
                                  productionMode,
                                  clientType,
                                  clientDbPath);

    // do not do backups for convo or contacts for Welcome Bot
    WickrCore::WickrRuntime::taskSvcEnableContactBackup(false);
    WickrCore::WickrRuntime::taskSvcEnableConvoBackup(false);

    if( !username.isEmpty() ) {
        WickrDBAdapter::setDBName( WickrDBAdapter::getDBName() + "." + username );
    }

    // If the user did not set the config file then lets try a default location
    if (wbConfigFile.isEmpty()) {
        wbConfigFile = searchConfigFile();
        if (wbConfigFile.isEmpty()) {
            qDebug() << "Cannot determine settings file!";
            exit(1);
        }
    }

    // get the settings file
    QSettings * settings = new QSettings(wbConfigFile, QSettings::NativeFormat, app);

    // Save the settings to the operation data
    operation->m_settings = settings;

    // Get the appropriate database location
    if (operation->databaseDir.isEmpty()) {
        settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
        QString dirName = settings->value(WBSETTINGS_DATABASE_DIRNAME, "").toString();
        settings->endGroup();

        if (dirName.isEmpty())
            operation->databaseDir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );
        else
            operation->databaseDir = dirName;
    }

    // Get the location of the Attachments directory
    if (operation->attachmentsDir.isEmpty()) {
        settings->beginGroup(WBSETTINGS_ATTACH_HEADER);
        QString dirName = settings->value(WBSETTINGS_ATTACH_DIRNAME, "").toString();
        settings->endGroup();

        if (dirName.isEmpty())
            operation->attachmentsDir = QString("%1/attachments").arg(QStandardPaths::writableLocation( QStandardPaths::DataLocation ));
        else
            operation->attachmentsDir = dirName;
    }
    if (!WBIOCommon::makeDirectory(operation->attachmentsDir)) {
        qDebug() << "WickrBot Server cannot make attachments directory:" << operation->attachmentsDir;
        return 1;
    }

    // Get the log file location/name
    if (operation->log_handler->getLogFile().isEmpty()) {
        settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
        QString fileName = settings->value(WBSETTINGS_LOGGING_FILENAME, "").toString();
        settings->endGroup();

        if (fileName.isEmpty())
            fileName = QStandardPaths::writableLocation( QStandardPaths::DataLocation ) + "/wickrio.log";

        // Check that can create the log file
        QFileInfo fileInfo(fileName);
        QDir dir;
        dir = fileInfo.dir();

        if (!WBIOCommon::makeDirectory(dir.path())) {
            qDebug() << "WickrBot Server cannot make log directory:" << dir;
            return 1;
        }

        operation->log_handler->setupLog(fileName);
    }

    // Set the output file if it is set
    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
    if (! curOutputFilename.isEmpty()) {
        operation->log_handler->logSetOutput(curOutputFilename);
    }
    settings->endGroup();

    /*
     * Get the user name associated with this account. This is needed for the
     * clients record for the run of this program.
     */
    if (! setProcessName) {
        settings->beginGroup(WBSETTINGS_USER_HEADER);
        QString username = settings->value(WBSETTINGS_USER_USERNAME, "").toString();
        settings->endGroup();

        if (username.isEmpty()) {
            qDebug() << "Username is not set";
            exit(1);
        }

        QFile appname(argv[0]);
        QFileInfo fi(appname);

        operation->processName = QString("%1.%2").arg(fi.fileName() ).arg(username);
    }

    // Parse the services settings
    settings->beginGroup(WBSETTINGS_SERVICES_HEADER);
    QString handleInbox = settings->value(WBSETTINGS_SERVICES_HANDLEINBOX, "").toString();
    if (handleInbox.isEmpty() || handleInbox.toLower() != "true") {
        operation->m_handleInbox = false;
    } else {
        operation->m_handleInbox = true;
    }

    // If the duration was not set by command line then check the ini file
    if (!setDuration) {
        operation->duration = settings->value(WBSETTINGS_SERVICES_DURATION, 0).toInt();
    }
    settings->endGroup();

    if (operation->duration != 0) {
        qDebug() << "Duration is set to " << operation->duration;
    }

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init(operation);

    // Create the receive details object
    WelcomeClientRxDetails *rxDetails = new WelcomeClientRxDetails(operation);

    /*
     * Start the WickrIO thread
     */
    WICKRBOT = new WickrIOClientMain(operation, rxDetails, WICKRBOT_SERVICE_ACTIONSVC);
    if (!WICKRBOT->parseSettings(settings)) {
        qDebug() << "Problem parsing Config file!";
        exit(1);
    }

    /*
     * When the WickrIOClientMain thread is started then create the IP
     * connection, so that other processes can stop this client.
     */
    QObject::connect(WICKRBOT, &WickrIOClientMain::signalStarted, [=]() {
        WickrIOClientRuntime::startIPC();
        WICKRBOT->setIPC(WickrIOClientRuntime::ipcSvc());
    });

    /*
     * When the login is successful create the HTTP listner to receive
     * the Web API requests.
     */
    QObject::connect(WICKRBOT, &WickrIOClientMain::signalLoginSuccess, [=]() {
        /*
         * Configure and start the TCP listener
         */
        settings->beginGroup(WBSETTINGS_LISTENER_HEADER);
        requestHandler = new RequestHandler(operation, app);
        httpListener = new stefanfrings::HttpListener(settings,requestHandler,app);
        settings->endGroup();
    });
    WICKRBOT->start();

    int retval = app->exec();

    /*
     * Shutdown the wickrIO Client Runtime
     */
    WickrIOClientRuntime::shutdown();

    httpListener->deleteLater();
    requestHandler->deleteLater();
    QCoreApplication::processEvents();

    WICKRBOT->deleteLater();
    QCoreApplication::processEvents();

    operation->deleteLater();

    return retval;
}
