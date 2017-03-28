#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#include "wickrbotutils.h"
#include "wickrbotsettings.h"

#include "webserver.h"

WebServer::WebServer(QString rootDir, QSettings *settings, QObject *parent) :
    QObject(parent),
    m_rootDir(rootDir),
    m_settings(settings)
{
}

/**
 * @brief WebServer::saveSettings
 * @return
 */
bool WebServer::saveSettings()
{
    QString logname;

    logname = QString("%1/logs/%2.log").arg(m_dbDir).arg(m_appName);

    m_settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    m_settings->setValue(WBSETTINGS_LOGGING_FILENAME, logname);
    m_settings->endGroup();

    m_settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    m_settings->setValue(WBSETTINGS_DATABASE_DIRNAME, m_dbDir);
    m_settings->endGroup();

    m_settings->sync();
    return true;
}
