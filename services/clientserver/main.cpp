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
#include "wbio_common.h"
#include "server_common.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"

extern bool isVERSIONDEBUG();

#define DO_AS_SERVICE 1

#include "wickrioclientserverservice.h"

#ifdef Q_OS_LINUX
WickrIOClientServerService *curService;
#endif

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    QString filename;
    QString dirname;

#ifdef Q_OS_WIN
    dirname = QString("%1/%2/%3/logs")
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET);
#elif defined(Q_OS_LINUX)
    dirname = QString("/opt/%1/logs").arg(WBIO_GENERAL_TARGET);
#endif

    filename = QString("%1/%2.output").arg(dirname).arg(WBIO_CLIENTSERVER_TARGET);
    QFile output(filename);

    // Make sure the directory exists
    if (WBIOCommon::makeDirectory(dirname)) {
        output.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream outstream(&output);
        outstream << str << "\n";
        output.close();

        if (type == QtFatalMsg) {
            abort();
        }
    }
}

#ifdef Q_OS_LINUX
void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

    auto handler = [](int sig) ->void {
        if (curService != nullptr) {
            curService->stop();
        }
        qDebug() << "\nquit the application (user request signal = %d)" << sig;
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
    qInstallMessageHandler(redirectedOutput);
    bool systemd = false;
    int svcret;

    WickrIOClientServerService service(argc, argv);

#ifdef Q_OS_LINUX
    for (int i=0; i<argc; i++) {
        qDebug() << "ARG[" << i+1 << "] =" << argv[i];
        QString cmd(argv[i]);

        if( cmd.startsWith("-systemd") ) {
            systemd = true;
        }
    }

    if (systemd) {
        qDebug() << "Starting in systemd mode!";
        curService = &service;
        catchUnixSignals({SIGTSTP, SIGQUIT, SIGTERM});

        QCoreApplication *app = new QCoreApplication(argc, argv);
        service.start();
        svcret = app->exec();
        curService = nullptr;
    } else {
        svcret = service.exec();
    }
#else
    svcret = service.exec();
#endif

    qDebug() << "Leaving Service exec";
    return svcret;
}
