#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#include "wickrIOCommon.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"
#include "server_common.h"

#include "webserver.h"

extern bool isVERSIONDEBUG();

WebServer::WebServer(QString dbDir, QSettings *settings, QObject *parent) :
    QObject(parent),
    m_dbDir(dbDir),
    m_appName(WBIO_CLIENTSERVER_TARGET)
{
    m_settings = settings;
    getSettings();
}

/**
 * @brief WebServer::executableFile
 * This function will return the name of the executable file associated with the
 * WickrBot Web Server. Possible executable names are:
 *     wickrbot_webserver <-- version in build directory
 *     wickrbotserver     <-- installed version
 * Possible locations for the executable include the following:
 *     /usr/bin
 *     <app bin dir>
 * @return
 */
QString WebServer::executableFile() {
    QString binDir=QCoreApplication::applicationDirPath();
    QString fileName;

    // Try to find the target executable
#ifdef Q_OS_WIN
    fileName = QString("%1.exe").arg(WBIO_CLIENTSERVER_TARGET);
#else
    fileName = WBIO_CLIENTSERVER_TARGET;
#endif

    QStringList searchList;
    searchList.append(binDir);
    searchList.append(QDir::rootPath()+"/usr/bin");
    if (isVERSIONDEBUG()) {
        searchList.append(binDir+"/../../clientserver/debug");
    } else {
        searchList.append(binDir+"/../../clientserver/release");
    }

    QString retFile = WickrBotUtils::fileInList(fileName, searchList);

    return retFile;
}

void WebServer::getSettings()
{
    m_settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    m_dbDir = m_settings->value(WBSETTINGS_DATABASE_DIRNAME, "").toString();
    m_settings->endGroup();

    QString logname = QString("%1/logs/%2.log").arg(m_dbDir).arg(m_appName);
    QString outname = QString("%1/logs/%2.output").arg(m_dbDir).arg(m_appName);

    m_settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    m_logFileName = m_settings->value(WBSETTINGS_LOGGING_FILENAME, logname).toString();
    m_outFileName = m_settings->value(WBSETTINGS_LOGGING_OUTPUT_FILENAME, outname).toString();
    m_settings->endGroup();

}

bool WebServer::saveSettings()
{
    m_settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    m_settings->setValue(WBSETTINGS_LOGGING_FILENAME, m_logFileName);
    m_settings->setValue(WBSETTINGS_LOGGING_OUTPUT_FILENAME, m_outFileName);
    m_settings->endGroup();

    m_settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    m_settings->setValue(WBSETTINGS_DATABASE_DIRNAME, m_dbDir);
    m_settings->endGroup();

    m_settings->sync();
    return true;
}

bool WebServer::start()
{
    // Save the settings before proceeding
    saveSettings();

    // Start the client application for the specific client/user
    QStringList arguments;
    QString command(executableFile());

#ifdef Q_OS_WIN
#else
    // Check for the existence of the Config file.  Cannot continue if it does not exist
    QFile configFile(m_settings->fileName());
    if (!configFile.exists()) {
        qDebug() << "Error: config file does not exist for" << m_appName;
        return false;
    }

    arguments.append(QString("-config=%1").arg(m_settings->fileName()));
#endif

    QProcess exec;
    exec.setStandardOutputFile(m_outFileName, QIODevice::Append);
    if (exec.startDetached(command, arguments)) {
        qDebug() << "Started client for" << m_appName;
    } else {
        qDebug() << "Could NOT start client for" << m_appName;
        return false;
    }
    return true;
}
