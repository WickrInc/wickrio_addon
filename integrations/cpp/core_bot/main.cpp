#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QThread>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QtPlugin>

#include "wickrbotutils.h"
#include "operationdata.h"
#include "wickrbotmain.h"
#include "wickrbotsettings.h"
#include "bot_iface.h"
#include "coreClientConfigInfo.h"

WickrBotMain        *m_wbmain;
CoreOperationData   *operation = nullptr;

bool makeDirectory(QString dirname)
{
    QDir tmp(dirname);
    if (!tmp.exists()) {
        if (!tmp.mkpath(".")) {
            qDebug() << "Cannot make directory" << dirname;
            return false;
        }
    }
    return true;
}


static void
usage()
{
    qDebug() << "Usage:";
    qDebug() << "parse_command [-d] [-force] [-dbdir=<db directory>] [-attachdir=<attachment directory>] [-log=<logs directory>]" <<
                "[-qconfig=<config file>] [-qhost=<host:port>|<host>] [-quser=<queue username>] [-qpassword=<queue password>]" <<
                " [-duration=<secs to run>] [-appName=<name of parser>]";
    qDebug() << "The parse_command program will parse the input JSON file and send output to the designed location.";
    exit(0);
}

/** Search the configuration file */
QString
searchConfigFile() {
    QStringList searchList;
    searchList.append(QDir::currentPath());

    // Look for the ini file with the application name
    QString appName=QCoreApplication::applicationName();
    QString fileName(appName+".ini");

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    if (retFile.isEmpty()) {
        retFile = WickrBotUtils::fileInList(WBIO_CORE_BOT_INI, searchList);
        if (retFile.isEmpty()) {
            qFatal("Cannot find config file %s",qPrintable(fileName));
        }
    }
    return retFile;
}

/**
 * @brief parseConfigFile
 * This function will open the input config file and parse out the Rabbit MQ login
 * credentials. The contents of the config file should contain JSON format with the
 * "qhost", "quser" and "qpassword" string values.
 * @return True is returned if successful, false if there was a problem parsing the file
 */
bool parseConfigFile(QString qConfigFile, CoreOperationData *operation)
{
    QSettings settings(qConfigFile, QSettings::NativeFormat);

    /*
     * Parse the general settings: client name, and messages
     */
    settings.beginGroup(WBSETTINGS_GEN_HEADER);
    operation->m_botName = settings.value(WBSETTINGS_GEN_CLIENT, "").toString();
    settings.endGroup();

    if (operation->m_botName.isEmpty())
        return false;
    return true;
}

void redirectedOutput(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    //in this function, you can write the message to any stream!
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
    case QtInfoMsg:
        if (operation != nullptr && operation->log_handler != nullptr)
            operation->log_handler->output(str);
        break;
    case QtFatalMsg:
        if (operation != nullptr && operation->log_handler != nullptr)
            operation->log_handler->output(str);
//        abort();
    }
}

int main(int argc, char *argv[])
{

#ifdef Q_OS_LINUX
#ifdef WICKR_BETA
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-beta");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-beta/plugins");
#elif defined(WICKR_ALPHA)
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-alpha");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot-alpha/plugins");
#else
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot");
    QCoreApplication::addLibraryPath("/usr/lib/wio_welcome_bot/plugins");
#endif
#endif


    QCoreApplication a(argc, argv);
    QString appName;
    operation = new CoreOperationData();
    operation->log_handler->setupLog(QString("%1/core_bot.log").arg(QDir::currentPath()));
    operation->log_handler->logSetOutput(QString("%1/core_bot.output").arg(QDir::currentPath()));

    qInstallMessageHandler(redirectedOutput);

    a.setOrganizationName("Wickr, LLC");

    QString wbConfigFile("");

    for( int argidx = 1; argidx < argc; argidx++ ) {
        QString cmd(argv[argidx]);

        if ( cmd == "-?" || cmd == "-help" || cmd == "--help" ) {
            usage();
        } else if (cmd == "-d") {
            operation->debug = true;
        } else if (cmd == "-force") {
            operation->force = true;
        } else if (cmd.startsWith("-config=")) {
            wbConfigFile = cmd.remove("-config=");
        }
        else if(cmd.startsWith("-appName")){
            appName = cmd.remove("-appName=");
        }
    }
    a.setApplicationName(appName);
    // If the config file was not specified on command line then try to find it
    if (wbConfigFile.isEmpty()) {
        wbConfigFile = searchConfigFile();
        if (wbConfigFile.isEmpty()) {
            qDebug() << "Cannot determine settings file!";
            exit(1);
        }
    }

    // Parse the configuration file contents
    if (!parseConfigFile(wbConfigFile, operation)) {
        qDebug() << "Failed to configure properly!";
        exit(1);
    }

    // Setup the bot interface
    operation->m_botIface = new BotIface(operation->m_botName.toStdString());
    if (operation->m_botIface->init() != BotIface::SUCCESS) {
        qDebug() << "Could not initialize the Bot interface!";
        exit(1);
    }

    WICKRBOT = new WickrBotMain(operation);

    QObject::connect(WICKRBOT, &WickrBotMain::signalStarted, [=]() {
        qDebug() << "Main process starting!";
    });

    WICKRBOT->start();
    int retval = a.exec();

    //clean up resources
    WICKRBOT->deleteLater();

    return retval;
}

