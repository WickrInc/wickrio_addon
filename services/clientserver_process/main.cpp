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

extern bool isVERSIONDEBUG();

#define DO_AS_SERVICE 1

#include "wickrioclientserver.h"

#ifdef Q_OS_LINUX
WickrIOClientServer *curService;
#endif

LogHandler logs;

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{

    logs.output(str);
    if (type == QtFatalMsg) {
        abort();

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

    WBIOCommon::makeDirectory(dirname);
    logs.setupLog(logname);
    logs.logSetOutput(filename);
    qInstallMessageHandler(redirectedOutput);
    int svcret;


#ifdef Q_OS_LINUX
    for (int i=0; i<argc; i++) {
        qDebug() << "ARG[" << i+1 << "] =" << argv[i];
        QString cmd(argv[i]);
    }

    catchUnixSignals({SIGTSTP, SIGQUIT, SIGTERM});
#else
#endif

    QCoreApplication *app = new QCoreApplication(argc, argv);

    WICKRIOCLIENTSERVER = new WickrIOClientServer();
    WICKRIOCLIENTSERVER->start();

    svcret = app->exec();

    qDebug() << "Leaving Client Service";
    return svcret;
}
