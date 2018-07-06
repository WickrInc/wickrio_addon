#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>

#include <QSqlQuery>
#include <QSqlError>

#ifdef Q_OS_LINUX
#include <signal.h>
#include <unistd.h>
#endif

#include "operationdata.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"
#include "loghandler.h"

#define DO_AS_SERVICE 1

#include "wickrioclientserverprocess.h"
#include "wickrioprocesscommand.h"

#ifdef Q_OS_LINUX
WickrIOClientServerProcess *curService;
#endif

LogHandler logs;

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    if (type == QtMsgType::QtDebugMsg && str.startsWith("CONSOLE:")) {
        QString outstr = str.right(str.length()-8);
        QTextStream(stdout) << outstr << endl;
    } else {

        logs.output(str);
        if (type == QtFatalMsg) {
            abort();
        }
    }
}

#ifdef Q_OS_LINUX
void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

    auto handler = [](int sig) ->void {
        qDebug() << "\nquit the application: user request signal = " << sig;
        QCoreApplication::quit();
    };

    // all these signals will be ignored.
    for ( int sig : ignoreSignals )
        signal(sig, SIG_IGN);

    // each of these signals calls the handler (quits the QCoreApplication).
    for ( int sig : quitSignals )
        signal(sig, handler);
}
#endif

Q_IMPORT_PLUGIN(QSQLCipherDriverPlugin)

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{

    QString filename;
    QString dirname;
    QString logname;

#ifdef Q_OS_WIN
    dirname = QString("%1/%2/%3/logs")
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET);
#elif defined(Q_OS_LINUX)
    dirname = QString("/opt/%1/logs").arg(WBIO_GENERAL_TARGET);
#endif

    filename = QString("%1/%2.output").arg(dirname).arg(WBIO_CLIENTSERVER_TARGET);
    logname =  QString("%1/%2.log").arg(dirname).arg(WBIO_CLIENTSERVER_TARGET);

    bool debugOutput = false;
    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if (cmd == "-debug") {
            debugOutput = true;
        }
    }

    WBIOCommon::makeDirectory(dirname);
    logs.setupLog(logname);
    logs.logSetOutput(filename);

    if (!debugOutput)
        qInstallMessageHandler(redirectedOutput);
    int svcret;

    catchUnixSignals({SIGTSTP, SIGQUIT, SIGTERM});

    QCoreApplication *app = new QCoreApplication(argc, argv);

    OperationData *pOperation = new OperationData();
    pOperation->processName = WBIO_CLIENTSERVER_TARGET;

    WICKRIOCLIENTSERVERPROCESS = new WickrIOClientServerProcess(pOperation);
    WICKRIOCLIENTSERVERPROCESS->start();
    WICKRIOPROCESSCOMMAND = new WickrIOProcessCommand(pOperation);
    WICKRIOPROCESSCOMMAND->start();

    QObject::connect(WICKRIOPROCESSCOMMAND, &WickrIOProcessCommand::signalQuit,
                     WICKRIOCLIENTSERVERPROCESS, &WickrIOClientServerProcess::processFinished,
                     Qt::QueuedConnection);

    svcret = app->exec();

    qDebug() << "Leaving Client Service";
    return svcret;
}
