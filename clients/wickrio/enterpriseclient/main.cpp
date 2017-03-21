#include "wbio_common.h"
#include "wickrbotsettings.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

#include "session/wickrSession.h"
#include "user/wickrApp.h"
#include "common/wickrRuntime.h"

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"

#include "wickrIOClientRuntime.h"


#ifdef WICKR_PLUGIN_SUPPORT
Q_IMPORT_PLUGIN(WickrPlugin)
#endif

#ifdef Q_OS_MAC
#include "platforms/mac/extras/WickrAppDelegate-C-Interface.h"
extern void wickr_powersetup(void);
#endif

#include <httpserver/httplistener.h>

#include "wickrioeclientmain.h"
#include "wickrioipc.h"
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

void noDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

/** Search the configuration file */
QString searchConfigFile() {
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
        if (isVERSIONDEBUG()) {
            fileName = "WickrBotClientBeta.ini";
        } else {
            fileName = "WickrBotClient.ini";
        }

        retFile = WickrBotUtils::fileInList(fileName, searchList);

        if (retFile.isEmpty()) {
            qFatal("Cannot find config file %s",qPrintable(fileName));
        }
    }
    return retFile;
#endif
}

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        operation->output(str);
        break;
    case QtFatalMsg:
        operation->output(str);
        abort();
    }
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

int main(int argc, char *argv[])
{
    QCoreApplication *app = NULL;

    Q_INIT_RESOURCE(WickrIOEClient);

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
    QString appname = WBIO_ECLIENT_TARGET;
    QString orgname = WBIO_ORGANIZATION;

    wickrProductSetProductType(ClientVersionInfo::getProductType());
    WickrURLs::setDefaultBaseURL(ClientConfigurationInfo::DefaultBaseURL);

    qDebug() <<  appname << "System was booted" << WickrUtil::formatTimestamp(WickrAppClock::getBootTime());

    bool dbEncrypt = true;

    operation = new OperationData();
    operation->processName = WBIO_ECLIENT_TARGET;

    QString clientDbPath("");
    QString suffix;
    QString wbConfigFile("");
    bool setProcessName = false;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if( cmd.startsWith("-dbdir=") ) {
            operation->databaseDir = cmd.remove("-dbdir=");
        } else if (cmd.startsWith("-log=") ) {
            QString logFile = cmd.remove("-log=");
            operation->setupLog(logFile);
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

#ifdef Q_OS_LINUX
#if defined(WICKR_TARGET_PROD)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio/plugins");
#elif defined(WICKR_TARGET_QA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-qa/plugins");
#elif defined(WICKR_TARGET_BETA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-beta/plugins");
#elif defined(WICKR_TARGET_ALPHA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-alpha/plugins");
#else
    This is an issue, cannot set the library!
#endif
#endif

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

    // Wickr Runtime Environment (all applications include this line)
    WickrAppContext::initialize(clientDbPath);
    WickrCore::WickrRuntime::init(secureJson, isDebug);

    WickrDBAdapter::setDatabaseEncryptedStatus(dbEncrypt);

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
    if (operation->getLogFile().isEmpty()) {
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

        operation->setupLog(fileName);
    }

    // Set the output file if it is set
    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    QString curOutputFilename = settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, "").toString();
    if (! curOutputFilename.isEmpty()) {
        operation->logSetOutput(curOutputFilename);
        operation->log(QString("Redirecting output to %1").arg(curOutputFilename));
        qInstallMessageHandler(redirectedOutput);
    } else {
        operation->log("Output will go to normal output!");
    }
    settings->endGroup();

    /*
     * Get the user name associated with this account. This is needed for the
     * clients record for the run of this program.
     */
    if (! setProcessName) {
        settings->beginGroup(WBSETTINGS_USER_HEADER);
        QString username = settings->value(WBSETTINGS_USER_USER, "").toString();
        settings->endGroup();

        if (username.isEmpty()) {
            qDebug() << "User or password is not set";
            exit(1);
        }

        operation->processName = WBIOCommon::getClientProcessName(username);
    }

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init(operation);

    /*
     * Start the WickrIO thread
     */
    WICKRBOT = new WickrIOEClientMain(operation);
    if (!WICKRBOT->parseSettings(settings)) {
        qDebug() << "Problem parsing Config file!";
        exit(1);
    }

    /*
     * When the WickrIOEClientMain thread is started then create the IP
     * connection, so that other processes can stop this client.
     */
    QObject::connect(WICKRBOT, &WickrIOEClientMain::signalStarted, [=]() {
        WICKRBOTIPC = new WickrBotMainIPC(operation);
        WICKRBOTIPC->start();
        WICKRBOT->setIPC(WICKRBOTIPC);
    });

    /*
     * When the login is successful create the HTTP listner to receive
     * the Web API requests.
     */
    QObject::connect(WICKRBOT, &WickrIOEClientMain::signalLoginSuccess, [=]() {
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

    httpListener->deleteLater();
    requestHandler->deleteLater();
    QCoreApplication::processEvents();

    WICKRBOTIPC->deleteLater();
    WICKRBOT->deleteLater();
    QCoreApplication::processEvents();

    operation->deleteLater();
//    delete operation;

    return retval;
}
