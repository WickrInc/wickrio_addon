#include <QDebug>
#include <QPlainTextEdit>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "wbio_common.h"

#include "wickrIOBot.h"

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
#include "cmdMain.h"

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

    wickrProductSetProductType(ClientVersionInfo::getProductType());
    WickrURLs::setDefaultBaseURL(ClientConfigurationInfo::DefaultBaseURL);

    bool debugOutput = false;

    QString suffix;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd.startsWith("-suffix")) {
            suffix = cmd.remove("-suffix=");
            WickrUtil::setTestAccountMode(suffix);
        } else if (cmd == "-debug") {
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


    CmdMain mainCommands(app, argc, argv);
    mainCommands.runCommands();

    return 0;
}
