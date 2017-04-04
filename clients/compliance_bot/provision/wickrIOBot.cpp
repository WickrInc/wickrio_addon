#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"

#include "wbio_common.h"
#include "wickriotokens.h"
#include "cmdbase.h"

#include "wickrIOClientRuntime.h"
#include "wickrIOBot.h"
#include "wickrIOBootstrap.h"
#include "wickrioeclientmain.h"
#include "wickrbotsettings.h"

#include "session/wickrSession.h"
#include "user/wickrApp.h"
#include "common/wickrRuntime.h"

#include "WickrUtil.h"


WickrIOBot::WickrIOBot(QCoreApplication *app, int argc, char **argv) :
    m_app(app),
    m_argc(argc),
    m_argv(argv),
    m_dbEncrypt(true),
    m_provision(&m_client)
{
    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd.startsWith("-config=")) {
            m_wbConfigFile = cmd.remove("-config=");
        }
    }

    if( isVERSIONDEBUG() ) {
        for( int argidx = 1; argidx < argc; argidx++ ) {
            QString cmd(argv[argidx]);

            if( cmd == "-crypt" ) {
                m_dbEncrypt = true;
            }
            else if( cmd == "-nocrypt" ) {
                m_dbEncrypt = false;
            }
            else if( cmd == "-noexclusive" ) {
                WickrDBAdapter::setDatabaseExclusiveOpenStatus(false);
            }
        }
    }

}

bool
WickrIOBot::newBotCreate()
{
    /*
     * Get input from the user
     */
    if (!m_provision.runCommands()) {
        return false;
    }

    m_client.binary = WBIO_BOT_TARGET;

    // Load the bootstrap file
    QString bootstrapString = WickrIOBootstrap::readFile(m_provision.m_configFileName, m_provision.m_configPassword);

    if (bootstrapString == nullptr) {
        qDebug() << "CONSOLE:Cannot read the bootstrap file!";
        exit(1);
    }

    // Set the test account name, which is appended to make the device ID specific to the client
    WickrUtil::setTestAccountMode(m_client.name);

    if (m_clientDbPath.isEmpty()) {
        m_clientDbPath = QString("%1/clients/%2/client").arg(WBIO_DEFAULT_DBLOCATION).arg(m_client.name);

        QDir clientDb(m_clientDbPath);
        if (!clientDb.exists()) {
            QDir dir = QDir::root();
            dir.mkpath(m_clientDbPath);
        }
    }
    if (m_wbConfigFile.isEmpty()) {
        m_wbConfigFile = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_client.name);
    }

    // Wickr Runtime Environment (all applications include this line)
    WickrCore::WickrRuntime::init(m_argc, m_argv,
                                  ClientVersionInfo::getProductType(),
                                  ClientVersionInfo::getOrgName(),
                                  ClientVersionInfo::getAppName(),
                                  ClientConfigurationInfo::DefaultBaseURL,
                                  isVERSIONDEBUG(),
                                  m_clientDbPath);

    // Will need to save the bootstrap file once we get the real password
    WickrIOEClientMain::loadBootstrapString(bootstrapString);

    if( !m_client.name.isEmpty() ) {
        WickrDBAdapter::setDBName( WickrDBAdapter::getDBName() + "." + m_client.name );
    }

    QString logname = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_client.name);
    QString attachDir = QString(WBIO_CLIENT_ATTACHDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(m_client.name);

    /*
     * Start the wickrIO Client Runtime
     */
    WickrIOClientRuntime::init();

    /*
     * Start the WickrIO thread
     */
    if (m_client.onPrem) {
        QString networkToken = WickrCore::WickrRuntime::getEnvironmentMgr()->networkToken();

        WICKRBOT = new WickrIOEClientMain(&m_client, networkToken);
    } else {
        WICKRBOT = new WickrIOEClientMain(&m_client, m_provision.m_invitation);
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
        m_app->quit();
    });
    QObject::connect(WICKRBOT, &WickrIOEClientMain::signalLoginFailure, [=]() {
        qDebug() << "CONSOLE:Failed to register/login!";
        m_app->quit();
    });
    WICKRBOT->start();

    int retval = m_app->exec();

    if (WICKRBOT->loginSuccess()) {
        // save setup information to the settings file
        QSettings * settings = new QSettings(m_wbConfigFile, QSettings::NativeFormat, m_app);

        settings->beginGroup(WBSETTINGS_USER_HEADER);
        settings->setValue(WBSETTINGS_USER_USER, m_client.user);
        settings->setValue(WBSETTINGS_USER_PASSWORD, m_client.password);      //TODO: THIS NEEDS TO BE REMOVED
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
        QString bootstrapFilename = m_clientDbPath + "/bootstrap";
        if (!WickrIOBootstrap::encryptAndSave(bootstrapString, bootstrapFilename, m_client.password)) {
            qDebug() << "CONSOLE:Problem encrypting config reil";
        }

        // TODO: Need to add an entry into the client record table
        WickrIOClientDatabase *m_ioDB = new WickrIOClientDatabase(WBIO_DEFAULT_DBLOCATION);
        if (!m_ioDB->isOpen()) {
            qDebug() << "CONSOLE:cannot open database!";
        } else {
            if (!m_ioDB->insertClientsRecord(&m_client)) {
                qDebug() << "CONSOLE:cannot create client record!";
            } else {

#if 0
                QString processName = WBIOServerCommon::getClientProcessName(newClient);
#else
                QString processName = QString("%1.%2").arg(m_client.binary).arg(m_client.name);
#endif

                // Set the state of the client process to paused
                if (! m_ioDB->updateProcessState(processName, 0, PROCSTATE_PAUSED)) {
                    qDebug() << "CONSOLE:Could not create process state record!";
                }
            }
            m_ioDB->close();
        }
    }
    WICKRBOT->deleteLater();

    return true;
}


