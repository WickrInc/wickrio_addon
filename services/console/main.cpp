
#include <QApplication>
#include <QCoreApplication>
#include <QtPlugin>

#include "client.h"
#include "cmdmain.h"
#include "wickrIOServerCommon.h"
#include "wickrIOIPCRuntime.h"

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
    int returnval = 0;

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if( cmd == "-cmd" ) {
            cmdInterface = true;
        }
    }

    WickrIOIPCRuntime::init(WBIO_CONSOLE_TARGET, false);

    if (cmdInterface) {
        qInstallMessageHandler(redirectedOutput);
        QCoreApplication coreapp(argc, argv);

        coreapp.setOrganizationName(WBIO_ORGANIZATION);
        coreapp.setApplicationName(WBIO_CONSOLE_TARGET);

        CmdMain cmdmain;
        cmdmain.runCommands();
    } else {
        QApplication app(argc, argv);
        app.setOrganizationName(WBIO_ORGANIZATION);
        app.setApplicationName(WBIO_CONSOLE_TARGET);

        Client *client = new Client();
        client->show();
        returnval = app.exec();
        client->deleteLater();
    }

    WickrIOIPCRuntime::shutdown();

    return returnval;
}
