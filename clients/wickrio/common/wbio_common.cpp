#include <QStandardPaths>
#include <QDir>

#include "wbio_common.h"
#include "wickrbotsettings.h"

/**
 * @brief WBIOCommon::makeDirectory
 * This function will create the input directory, if it does not already exist
 * @param dirname
 * @return
 */
bool WBIOCommon::makeDirectory(QString dirname)
{
    QDir tmp(dirname);
    if (!tmp.exists()) {
        if (!tmp.mkpath(".")) {
            return false;
        }
    }
    return true;
}

/**
 * @brief WBIOCommon::getSettings
 * This function will return the QSettings object which is associated with the
 * settings/configuration for the WickrIO server applications.
 * @return
 */
QSettings *WBIOCommon::getSettings()
{
    QSettings *settings;

#ifdef Q_OS_WIN
    settings = new QSettings(QString(WBIO_SERVER_SETTINGS_FORMAT).arg(WBIO_ORGANIZATION).arg(WBIO_GENERAL_TARGET), QSettings::NativeFormat);
#else
    QString settingsFile = QString("/opt/%1/%2.ini").arg(WBIO_GENERAL_TARGET).arg(WBIO_GENERAL_TARGET);
    settings = new QSettings(settingsFile, QSettings::NativeFormat);
#endif

    return settings;
}

/**
 * @brief WBIOCommon::getDBLocation
 * This function will return the default database location for the WickrIO
 * server based applications.
 * @return
 */
QString WBIOCommon::getDBLocation()
{
    QString dbLocation;
    QSettings *settings = WBIOCommon::getSettings();

    settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    dbLocation = settings->value(WBSETTINGS_DATABASE_DIRNAME, "").toString();
    if (dbLocation.isEmpty()) {
#ifdef Q_OS_WIN
        dbLocation = QString("%1/%2/%3")
                .arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                .arg(WBIO_ORGANIZATION)
                .arg(WBIO_GENERAL_TARGET);
#else
        dbLocation = QString("/opt/%1").arg(WBIO_GENERAL_TARGET);
#endif
        settings->setValue(WBSETTINGS_DATABASE_DIRNAME, dbLocation);
        settings->sync();
    }
    settings->endGroup();

    WBIOCommon::makeDirectory(dbLocation);
    return dbLocation;
}

QString
WBIOCommon::getClientProcessName(QString name)
{
    QString processName;
    if (name.isEmpty()) {
        processName = WBIO_CLIENT_PROCESS;
    } else {
        processName = QString("%1.%2").arg(WBIO_CLIENT_PROCESS).arg(name);
    }
    return processName;
}
