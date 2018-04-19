#include <QDebug>
#include <QPlainTextEdit>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "complianceClientConfigInfo.h"

#include "wickrIOCommon.h"

#include "wickrIOBot.h"
#include "wickrIOConsole.h"

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
#include "wickrIOIPCService.h"
#include "wickrbotutils.h"
#include "cmdMain.h"
#include "operationdata.h"

OperationData           *operation = NULL;
WickrIOIPCService         *rxIPC;
WickrIOConsoleService   *consoleSvc;

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

    QString appname = WBIO_PROVISION_TARGET;
    QString orgname = WBIO_ORGANIZATION;

    operation = new OperationData();
    operation->processName = WBIO_PROVISION_TARGET;
    operation->databaseDir = WBIO_DEFAULT_DBLOCATION;
    operation->m_botDB = new WickrIOClientDatabase(operation->databaseDir);

    if (operation->m_botDB->isOpen()) {
        operation->m_botDB->updateProcessState(WBIO_PROVISION_TARGET, QCoreApplication::applicationPid(), PROCSTATE_RUNNING);
    }

    wickrProductSetProductType(PRODUCT_TYPE_BOT);
    WickrURLs::setDefaultBaseURLs(ClientConfigurationInfo::DefaultBaseURL, ClientConfigurationInfo::DefaultDirSearchBaseURL);

    bool debugOutput = false;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd == "-debug") {
            debugOutput = true;
        }
    }


    if (!debugOutput)
        qInstallMessageHandler(redirectedOutput);

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

    rxIPC = new WickrIOIPCService();
    rxIPC->startIPC(operation);


    consoleSvc = new WickrIOConsoleService(app, argc, argv, operation, rxIPC);
    consoleSvc->startConsole();

    app->exec();

    if (operation->m_botDB->isOpen()) {
        operation->m_botDB->updateProcessState(WBIO_PROVISION_TARGET, 0, PROCSTATE_DOWN);
    }

    rxIPC->deleteLater();
    consoleSvc->deleteLater();

    return 0;
}
