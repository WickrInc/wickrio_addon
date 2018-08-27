#include "wickrIOCommon.h"
#include "wickrbotsettings.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtPlugin>
#include <QLibraryInfo>

#include "session/wickrSession.h"
#include "user/wickrApp.h"
#include "common/wickrRuntime.h"

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "testClientConfigInfo.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOIPCRuntime.h"
#include "testClientRxDetails.h"
#include "wickrIOJScriptService.h"

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

void
dropOutput(QtMsgType type, const QMessageLogContext &, const QString & )
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        break;
    case QtFatalMsg:
        abort();
    }
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

int main(int argc, char *argv[])
{
    QCoreApplication *app = NULL;

    Q_INIT_RESOURCE(wickrio_bot);

    // Setup appropriate library values based on Beta or Production client
    QByteArray secureJson;
    if (isVERSIONDEBUG()) {
        secureJson = "secex_json2:Fq3&M1[d^,2P";
    } else {
        secureJson = "secex_json:8q$&M4[d^;2R";
    }

    QString appname = WBIO_CLIENT_TARGET;
    QString orgname = WBIO_ORGANIZATION;

#if defined(WICKR_MESSENGER)
    wickrProductSetProductType(PRODUCT_TYPE_MESSENGER);
#else
    wickrProductSetProductType(PRODUCT_TYPE_BOT);
#endif
    WickrURLs::setDefaultBaseURLs(ClientConfigurationInfo::DefaultBaseURL,
                                  ClientConfigurationInfo::DefaultDirSearchBaseURL);

    operation = new OperationData();

    QString clientDbPath("");
    QString wbConfigFile("");
    QString argOutputFile;
    QString userName;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if( cmd.startsWith("-dbdir=") ) {
            operation->databaseDir = cmd.remove("-dbdir=");
        } else if (cmd.startsWith("-debug")) {
            operation->debug = true;
        } else if (cmd.startsWith("-log=") ) {
            QString logFile = cmd.remove("-log=");
            operation->log_handler->setupLog(logFile);
        } else if (cmd.startsWith("-clientdbdir=")) {
            clientDbPath = cmd.remove("-clientdbdir=");
        } else if (cmd.startsWith("-config=")) {
            wbConfigFile = cmd.remove("-config=");
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
        } else if (cmd.startsWith("-clientname")) {
            operation->wickrID = cmd.remove("-clientname=");
            userName = operation->wickrID;
            userName.replace("@", "_");
            wbConfigFile = QString(WBIO_CLIENT_SETTINGS_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(userName);
            clientDbPath = QString(WBIO_CLIENT_DBDIR_FORMAT)
                    .arg(WBIO_DEFAULT_DBLOCATION)
                    .arg(userName);
            argOutputFile = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(userName);
        }
    }

    // Drop all output for now
    if (!operation->debug)
        qInstallMessageHandler(dropOutput);

    if( isVERSIONDEBUG() ) {
        for( int argidx = 1; argidx < argc; argidx++ ) {
            QString cmd(argv[argidx]);

            if( cmd == "-noexclusive" ) {
                WickrDBAdapter::setDatabaseExclusiveOpenStatus(false);
            }
        }
    }

    qDebug() <<  appname << "System was booted" << WickrUtil::formatTimestamp(WickrAppClock::getBootTime());

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

    if (operation->wickrID.isEmpty()) {
        settings->beginGroup(WBSETTINGS_USER_HEADER);
        operation->wickrID = settings->value(WBSETTINGS_USER_USER, "").toString();
        settings->endGroup();

        if (operation->wickrID.isEmpty()) {
            qDebug() << "User ID is not set in settings file!";
            exit(1);
        }
    }

    if (userName.isEmpty()) {
        settings->beginGroup(WBSETTINGS_USER_HEADER);
        userName = settings->value(WBSETTINGS_USER_USERNAME, "").toString();
        settings->endGroup();
    }

    WickrDBAdapter::setDBName( WickrDBAdapter::getDBName() + "." + operation->wickrID );

    // Product Type: for legacy and testing purposes we will support using client users
    int productType;

    // see if the product type is in the settings
    settings->beginGroup(WBSETTINGS_USER_HEADER);
    QString userpt_string = settings->value(WBSETTINGS_USER_PRODUCTTYPE, WBSETTINGS_USER_PT_DEFAULT).toString();
    settings->endGroup();
    if (userpt_string == WBSETTINGS_USER_PT_LEGACY) {
#if defined(WICKR_MESSENGER)
        productType = PRODUCT_TYPE_MESSENGER;
#elif defined(WICKR_ENTERPRISE)
        productType = PRODUCT_TYPE_PRO;
#else
        productType = PRODUCT_TYPE_PRO;
#endif
    } else {
#if defined(WICKR_MESSENGER)
        productType = PRODUCT_TYPE_MESSENGER;
#else
        productType = PRODUCT_TYPE_BOT;
#endif
    }

    // Wickr Runtime Environment (all applications include this line)
    WickrCore::WickrRuntime::init(argc, argv,
                                  productType,
                                  ClientVersionInfo::getOrgName(),
                                  appname,
                                  ClientConfigurationInfo::DefaultBaseURL,
                                  ClientConfigurationInfo::DefaultDirSearchBaseURL,
                                  productionMode,
                                  clientType,
                                  clientDbPath);



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

        if (dirName.isEmpty()) {
            operation->attachmentsDir = QString(WBIO_CLIENT_ATTACHDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client.name);

        }

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

        if (fileName.isEmpty()) {
            fileName = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(userName);
            settings->setValue(WBSETTINGS_LOGGING_FILENAME, fileName);
            settings->endGroup();
            settings->sync();
        } else {
            settings->endGroup();
        }

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
    if (!argOutputFile.isEmpty()) {
        operation->log_handler->logSetOutput(argOutputFile);
    } else {
        settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
        QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();

        if (curOutputFilename.isEmpty()) {
            curOutputFilename = QString(WBIO_CLIENT_OUTFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(userName);
            settings->setValue(WBSETTINGS_LOGGING_OUTPUT_FILENAME, curOutputFilename);
            settings->endGroup();
            settings->sync();
        } else {
            settings->endGroup();
        }

        operation->log_handler->logSetOutput(curOutputFilename);
    }

    /*
     * Get the user name associated with this account. This is needed for the
     * clients record for the run of this program.
     */
    if (operation->processName.isEmpty()) {
        if (userName.isEmpty()) {
            qDebug() << "Username field is not set";
            exit(1);
        }

        QFile appname(argv[0]);
        QFileInfo fi(appname);

        operation->processName = QString("%1.%2").arg(fi.fileName() ).arg(userName);
    }

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init(operation);
    WickrIOClientRuntime::setFileSendCleanup(true);
    WickrIOIPCRuntime::init(operation);

    /*
     * Start the Javascript service and attach to the Client Runtime
     */
    WickrIOJScriptService *jsSvc = new WickrIOJScriptService();
    if (!WickrIOClientRuntime::addService(jsSvc)) {
        qDebug() << "Could not start the JavaScript service!";
    }

    // Create the receive details object
    TestClientRxDetails *rxDetails = new TestClientRxDetails(operation);

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
        WickrIOIPCRuntime::startIPC();
        WICKRBOT->setIPC(WickrIOIPCRuntime::ipcSvc());
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


        /*
         * Start the Integration software if there is any configured
         */
        WickrIOJScriptService *jsSvc = (WickrIOJScriptService*)WickrIOClientRuntime::findService(WickrIOJScriptService::jsServiceBaseName);
        if (jsSvc)
            jsSvc->startScript();

    });
    WICKRBOT->start();



    int retval = app->exec();

    /*
     * Shutdown the wickrIO Client Runtime
     */
    WickrIOIPCRuntime::shutdown();
    WickrIOClientRuntime::shutdown();

    httpListener->deleteLater();
    requestHandler->deleteLater();
    QCoreApplication::processEvents();

    WICKRBOT->deleteLater();
    QCoreApplication::processEvents();

    operation->deleteLater();

    return retval;
}
