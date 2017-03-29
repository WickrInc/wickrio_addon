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

#ifdef WICKR_PLUGIN_SUPPORT
Q_IMPORT_PLUGIN(WickrPlugin)
#endif

#ifdef Q_OS_MAC
#include "platforms/mac/extras/WickrAppDelegate-C-Interface.h"
extern void wickr_powersetup(void);
#endif

#include "wickrioeclientmain.h"
#include "wickrioipc.h"
#include "wickrbotutils.h"
#include "cmdProvisioning.h"

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

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

int main(int argc, char *argv[])
{
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

    QString appname = WBIO_CLIENT_TARGET;
    QString orgname = WBIO_ORGANIZATION;

    wickrProductSetProductType(ClientVersionInfo::getProductType());
    WickrURLs::setDefaultBaseURL(ClientConfigurationInfo::DefaultBaseURL);

    bool dbEncrypt = true;

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

    // Get input from the user
    if (!provisioningInput.runCommands()) {
        exit(1);
    }

    // Load the bootstrap file
    // TODO: Need to save an encrypted version of the file
    WickrIOEClientMain::loadBootstrapFile(provisioningInput.m_configFileName, provisioningInput.m_configPassword);

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

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init();

    /*
     * Start the WickrIO thread
     */
    if (client.onPrem) {
        QString networkToken = WickrCore::WickrRuntime::getEnvironmentMgr()->networkToken();

        WICKRBOT = new WickrIOEClientMain(client.user, client.password, networkToken, true);
    } else {
        WICKRBOT = new WickrIOEClientMain(client.user, client.password, provisioningInput.m_invitation);
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
        qDebug() << "Successfully logged in as new user!";
        qDebug() << "Our work is done here, logging off!";
        app->quit();
    });
    WICKRBOT->start();

    int retval = app->exec();

//    QCoreApplication::processEvents();

    WICKRBOT->deleteLater();
//    QCoreApplication::processEvents();

    return retval;
}
