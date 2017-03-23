#include <QDebug>
#include <QString>

#include "inittest.h"
#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "common/wickrUtil.h"
#include "Wickr/WickrProduct.h"
#include "session/wickrAppClock.h"
#include "wickrapplication.h"
#include "common/wickrRuntime.h"
#include "wbio_common.h"

InitTest::InitTest(int argc, char **argv, QObject *parent) :
    QObject(parent),
    m_argc(argc),
    m_argv(argv),
    m_operation(nullptr)
{
}

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

void InitTest::initTestCase()
{
    QCoreApplication *app = NULL;

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

    m_operation = new OperationData();
    m_operation->processName = WBIO_ECLIENT_TARGET;

    QString clientDbPath("");
    QString suffix;
    QString wbConfigFile("");
    bool setProcessName = false;

    for( int argidx = 1; argidx < m_argc; argidx++ ) {
        QString cmd(m_argv[argidx]);

        if( cmd.startsWith("-dbdir=") ) {
            m_operation->databaseDir = cmd.remove("-dbdir=");
        } else if (cmd.startsWith("-log=") ) {
            QString logFile = cmd.remove("-log=");
            m_operation->setupLog(logFile);
        } else if (cmd.startsWith("-clientdbdir=")) {
            clientDbPath = cmd.remove("-clientdbdir=");
        } else if (cmd.startsWith("-config=")) {
            wbConfigFile = cmd.remove("-config=");
        } else if (cmd.startsWith("-suffix")) {
            suffix = cmd.remove("-suffix=");
            WickrUtil::setTestAccountMode(suffix);
        } else if (cmd.startsWith("-force") ) {
            // Force the WickBot Client to run, regardless of the state in the database
            m_operation->force = true;
        } else if (cmd.startsWith("-rcv")) {
            QString temp = cmd.remove("-rcv=");
            if (temp.compare("on", Qt::CaseInsensitive) ||
                temp.compare("true", Qt::CaseInsensitive)) {
                m_operation->receiveOn =true;
            }
        } else if (cmd.startsWith("-processname")) {
            m_operation->processName = cmd.remove("-processname=");
            setProcessName = true;
        }
    }

    app = new QCoreApplication(m_argc, m_argv);

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
}

void InitTest::testArea()
{
    QCOMPARE(2.0, 3.14);
    QCOMPARE(314.0, 314.0);
}

void InitTest::testRadius()
{
    QCOMPARE(2.0, 1.0);
    QCOMPARE(10.0, 10.0);
}

void InitTest::cleanupTestCase()
{
    WickrCore::WickrRuntime::shutdown();
}
