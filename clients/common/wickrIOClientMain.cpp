#include <QCoreApplication>
#include "wickrIOClientMain.h"
#include "wickrbotutils.h"
#include "operationdata.h"
#include "wickrIOClientRuntime.h"

void
WickrIOClientMain::noDebugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

/** Search the configuration file */
QString
WickrIOClientMain::searchConfigFile() {
#ifdef Q_OS_WIN
    return QString(WBIO_SERVER_SETTINGS_FORMAT)
            .arg(QCoreApplication::organizationName())
            .arg(QCoreApplication::applicationName());
#else
    // Setup the list of locations to search for the ini file
    QString filesdir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );

    QStringList searchList;
    searchList.append(filesdir);

    // Look for the ini file with the application name
    QString appName=QCoreApplication::applicationName();
    QString fileName(appName+".ini");

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    if (retFile.isEmpty()) {
        qFatal("Cannot find config file %s",qPrintable(fileName));
    }
    return retFile;
#endif
}

void
WickrIOClientMain::redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        break;
    case QtFatalMsg:
        WickrIOClientRuntime::operationData()->output(str);
        abort();
    }
}

