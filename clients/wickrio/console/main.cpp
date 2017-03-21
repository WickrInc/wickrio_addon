
#include <QApplication>
#include <QCoreApplication>
#include "client.h"
#include "cmdmain.h"
#include "wbio_common.h"

extern bool isVERSIONDEBUG();

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
    bool cmdInterface = false;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if( cmd == "-cmd" ) {
            cmdInterface = true;
        }
    }

    if (cmdInterface) {
        qInstallMessageHandler(redirectedOutput);
        QCoreApplication coreapp(argc, argv);

#if defined(Q_OS_LINUX)
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
        coreapp.setOrganizationName(WBIO_ORGANIZATION);
        coreapp.setApplicationName(WBIO_CONSOLE_TARGET);

        CmdMain cmdmain;
        cmdmain.runCommands();
    } else {
        QApplication app(argc, argv);
#if defined(Q_OS_LINUX)
#ifdef VERSIONDEBUG
        QApplication::addLibraryPath("/usr/lib/wickrio-beta/plugins");
#else
        QApplication::addLibraryPath("/usr/lib/wickrio/plugins");
#endif
#endif
        app.setOrganizationName(WBIO_ORGANIZATION);
        app.setApplicationName(WBIO_CONSOLE_TARGET);

        Client *client = new Client();
        client->show();
        int returnval = app.exec();
        client->deleteLater();

        return returnval;
    }
}
