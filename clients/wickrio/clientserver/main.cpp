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

#include "operationdata.h"
#include "wbio_common.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"

extern bool isVERSIONDEBUG();

#define DO_AS_SERVICE 1

#include "wickrioclientserverservice.h"

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

    for (int i=0; i<argc; i++) {
        qDebug() << "ARG[" << i+1 << "] =" << argv[i];
    }

    WickrIOClientServerService service(argc, argv);

#ifdef Q_OS_LINUX
    QCoreApplication *app = new QCoreApplication(argc, argv);
    service.start();
    int svcret = app->exec();
#else
    int svcret = service.exec();
#endif

    qDebug() << "Leaving Service exec";
    return svcret;
}
