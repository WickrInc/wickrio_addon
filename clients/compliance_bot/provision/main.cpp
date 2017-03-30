#include "wbio_common.h"
#include "wickrbotsettings.h"

#include <QDebug>
#include <QPlainTextEdit>
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

#include "wickrIOClientRuntime.h"
#include "cmdProvisioning.h"
#include "wickrIOBootstrap.h"

#ifdef WICKR_PLUGIN_SUPPORT
Q_IMPORT_PLUGIN(WickrPlugin)
#endif

#ifdef Q_OS_MAC
#include "platforms/mac/extras/WickrAppDelegate-C-Interface.h"
extern void wickr_powersetup(void);
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

#include "wickrioeclientmain.h"
#include "wickrioipc.h"
#include "wickrbotutils.h"

WickrIOClients  client;
CmdProvisioning provisioningInput(&client);

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

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    if (type == QtMsgType::QtDebugMsg) {
        if (str.startsWith("CONSOLE:")) {
            QString outstr = str.right(str.length()-8);
            QTextStream(stdout) << outstr << endl;
        }
    }
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    if (getuid()) {
        qDebug() << "You must be root to run this program!";
        exit(0);
    }
#endif

    QCoreApplication *app = NULL;

    Q_INIT_RESOURCE(provisioning);

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

    QString appname = WBIO_PROVISION_TARGET;
    QString orgname = WBIO_ORGANIZATION;

    wickrProductSetProductType(ClientVersionInfo::getProductType());
    WickrURLs::setDefaultBaseURL(ClientConfigurationInfo::DefaultBaseURL);

    bool dbEncrypt = true;
    bool debugOutput = false;

    QString clientDbPath("");
    QString suffix;
    QString wbConfigFile("");

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd.startsWith("-clientdbdir=")) {
            clientDbPath = cmd.remove("-clientdbdir=");
        } else if (cmd.startsWith("-config=")) {
            wbConfigFile = cmd.remove("-config=");
        } else if (cmd.startsWith("-suffix")) {
            suffix = cmd.remove("-suffix=");
            WickrUtil::setTestAccountMode(suffix);
        } else if (cmd == "-debug") {
            debugOutput = true;
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
            else if( cmd == "-noexclusive" ) {
                WickrDBAdapter::setDatabaseExclusiveOpenStatus(false);
            }
        }
    }

    if (!debugOutput)
        qInstallMessageHandler(redirectedOutput);

    /*
     * Get input from the user
     */
    if (!provisioningInput.runCommands()) {
        exit(1);
    }

    client.binary = WBIO_BOT_TARGET;

    // Load the bootstrap file
    QString bootstrapString = WickrIOBootstrap::readFile(provisioningInput.m_configFileName, provisioningInput.m_configPassword);

    if (bootstrapString == nullptr) {
        qDebug() << "CONSOLE:Cannot read the bootstrap file!";
        exit(1);
    }
    // Will need to save the bootstrap file once we get the real password
    WickrIOEClientMain::loadBootstrapString(bootstrapString);


    if (clientDbPath.isEmpty()) {
        clientDbPath = QString("%1/clients/%2/client").arg(WBIO_DEFAULT_DBLOCATION).arg(client.name);

        QDir clientDb(clientDbPath);
        if (!clientDb.exists()) {
            QDir dir = QDir::root();
            dir.mkpath(clientDbPath);
        }
    }
    if (wbConfigFile.isEmpty()) {
        wbConfigFile = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client.name);
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

    // Wickr Runtime Environment (all applications include this line)
    WickrAppContext::initialize(clientDbPath);
    WickrCore::WickrRuntime::init(secureJson, isDebug);

    WickrDBAdapter::setDatabaseEncryptedStatus(dbEncrypt);

    if( !client.name.isEmpty() ) {
        WickrDBAdapter::setDBName( WickrDBAdapter::getDBName() + "." + client.name );
    }

    QString logname = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client.name);
    QString attachDir = QString(WBIO_CLIENT_ATTACHDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(client.name);

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init();

    /*
     * Start the WickrIO thread
     */
    if (client.onPrem) {
        QString networkToken = WickrCore::WickrRuntime::getEnvironmentMgr()->networkToken();

        WICKRBOT = new WickrIOEClientMain(&client, networkToken);
    } else {
        WICKRBOT = new WickrIOEClientMain(&client, provisioningInput.m_invitation);
    }

    /*
     * When the WickrIOEClientMain thread is started then create the IP
     * connection, so that other processes can stop this client.
     */
    QObject::connect(WICKRBOT, &WickrIOEClientMain::signalStarted, [=]() {
    });

    /*
     * When the login is successful create the HTTP listner to receive
     * the Web API requests.
     */
    QObject::connect(WICKRBOT, &WickrIOEClientMain::signalLoginSuccess, [=]() {
        qDebug() << "CONSOLE:Successfully logged in as new user!";
        qDebug() << "CONSOLE:Our work is done here, logging off!";
        app->quit();
    });
    WICKRBOT->start();

    int retval = app->exec();

    // save setup information to the settings file
    QSettings * settings = new QSettings(wbConfigFile, QSettings::NativeFormat, app);

    settings->beginGroup(WBSETTINGS_USER_HEADER);
    settings->setValue(WBSETTINGS_USER_USER, client.user);
    settings->setValue(WBSETTINGS_USER_PASSWORD, client.password);      //TODO: THIS NEEDS TO BE REMOVED
    settings->endGroup();

#ifdef Q_OS_WIN
    QString dbLocation = QString("%1/%2/%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET);
#else
    QString dbLocation = QString("%1").arg(WBIO_DEFAULT_DBLOCATION);
#endif

    settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    settings->setValue(WBSETTINGS_DATABASE_DIRNAME, dbLocation);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    settings->setValue(WBSETTINGS_LOGGING_FILENAME, logname);
    settings->endGroup();

#if 0
    settings->beginGroup(WBSETTINGS_LISTENER_HEADER);
    settings->setValue(WBSETTINGS_LISTENER_PORT, newClient->port);
    // If using localhost interface then do not need the host entry in the settings
    if (newClient->iface == "localhost") {
        settings->remove(WBSETTINGS_LISTENER_IF);
    } else {
        settings->setValue(WBSETTINGS_LISTENER_IF, newClient->iface);
    }
    // If is HTTPS then save the SSL settings, otherwise remove them
    if (newClient->isHttps) {
        settings->setValue(WBSETTINGS_LISTENER_SSLKEY, newClient->sslKeyFile);
        settings->setValue(WBSETTINGS_LISTENER_SSLCERT, newClient->sslCertFile);
    } else {
        settings->remove(WBSETTINGS_LISTENER_SSLKEY);
        settings->remove(WBSETTINGS_LISTENER_SSLCERT);
    }
    settings->endGroup();
#endif
    settings->beginGroup(WBSETTINGS_ATTACH_HEADER);
    settings->setValue(WBSETTINGS_ATTACH_DIRNAME, attachDir);
    settings->endGroup();


    settings->sync();



    // Need to save the bootstrap file
    QString bootstrapFilename = clientDbPath + "/bootstrap";
    if (!WickrIOBootstrap::encryptAndSave(bootstrapString, bootstrapFilename, client.password)) {
        qDebug() << "CONSOLE:Problem encrypting config reil";
    }

    // TODO: Need to add an entry into the client record table
    WickrIOClientDatabase *m_ioDB = new WickrIOClientDatabase(WBIO_DEFAULT_DBLOCATION);
    if (!m_ioDB->isOpen()) {
        qDebug() << "CONSOLE:cannot open database!";
    } else if (!m_ioDB->insertClientsRecord(&client)) {
        qDebug() << "CONSOLE:cannot create client record!";
        return retval;
    } else {

#if 0
        QString processName = WBIOServerCommon::getClientProcessName(newClient);
#else
        QString processName = QString("%1.%2").arg(client.binary).arg(client.name);
#endif

        // Set the state of the client process to paused
        if (! m_ioDB->updateProcessState(processName, 0, PROCSTATE_PAUSED)) {
            qDebug() << "CONSOLE:Could not create process state record!";
        }
    }

    WICKRBOT->deleteLater();

    return retval;
}
