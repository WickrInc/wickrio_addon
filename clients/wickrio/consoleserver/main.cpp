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
#include "consoleserverservice.h"

extern bool isVERSIONDEBUG();

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

    filename = QString("%1/%2.output").arg(dirname).arg(WBIO_CONSOLESERVER_TARGET);
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

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(redirectedOutput);

    WickrIOConsoleServerService service(argc, argv);
    int svcret = service.exec();
    qDebug() << "Leaving Service exec";
    return svcret;
}
