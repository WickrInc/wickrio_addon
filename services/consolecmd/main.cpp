
#include <QCoreApplication>
#include "cmdmain.h"
#include "wickrIOServerCommon.h"

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
    qInstallMessageHandler(redirectedOutput);
    QCoreApplication coreapp(argc, argv);

    coreapp.setOrganizationName(WBIO_ORGANIZATION);
    coreapp.setApplicationName(WBIO_CONSOLE_TARGET);


    QLibrary library("/opt/WickrIODebug/integrations/software/testIntegration/libtestIntegration.so");
    if (!library.load()) {
        qDebug() << "Could not load testintegration library";
    } else {
        qDebug() << "testintegration library loaded";
        typedef QString (*TestInputfunction)(QString input);
        TestInputfunction tf = (TestInputfunction)library.resolve("processInput");
        if (tf) {
            QString output = tf("This is a test message");
            if (output.isEmpty()) {
                qDebug() << "CONSOLE:received empty string";
            } else {
                qDebug() << "CONSOLE:Received:" << output;
            }
        }
    }



    CmdMain cmdmain;
    if (argc > 1) {
        QString commands(argv[1]);
        cmdmain.runCommands(commands);
    } else {
        cmdmain.runCommands();
    }
}
