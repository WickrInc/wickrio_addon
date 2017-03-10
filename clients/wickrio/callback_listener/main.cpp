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
#include "cmdhandler.h"

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
#if defined(WICKR_TARGET_PROD)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio/plugins");
#elif defined(WICKR_TARGET_PREVIEW)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-prev/plugins");
#elif defined(WICKR_TARGET_BETA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-beta/plugins");
#elif defined(WICKR_TARGET_ALPHA)
    QCoreApplication::addLibraryPath("/usr/lib/wickrio-alpha/plugins");
#else
    This is an issue, cannot set the library!
#endif
#endif

    QCoreApplication *app;
    app = new QCoreApplication(argc, argv);

    if (argc != 2) {
        QFile file(argv[0]);
        QFileInfo fileInfo(file.fileName());

        qDebug() << "Usage:" << qPrintable(fileInfo.fileName()) << "<port number>";
    } else {
        int port;
        QString portStr = QString(argv[1]);
        port = portStr.toInt();

        QSettings *settings;
        settings = new QSettings("callback_listener.ini", QSettings::NativeFormat, NULL);

        settings->setValue("port", port);
    //    settings->setValue("host", "localhost");
        settings->sync();

        CmdHandler *cmdHandler = new CmdHandler(NULL);
        HttpListener *m_listener = new HttpListener(settings, cmdHandler, NULL);

        return app->exec();
    }
}
