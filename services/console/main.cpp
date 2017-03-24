
#include <QApplication>
#include <QCoreApplication>
#include "client.h"
#include "cmdmain.h"
#include "server_common.h"

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
        int returnval = app.exec();
        client->deleteLater();

        return returnval;
    }
}
