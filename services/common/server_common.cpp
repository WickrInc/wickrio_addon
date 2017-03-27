#include <QStandardPaths>
#include <QDir>

#include "wbio_common.h"
#include "server_common.h"
#include "wickrbotsettings.h"

/**
 * @brief WBIOCommon::getSettings
 * This function will return the QSettings object which is associated with the
 * settings/configuration for the WickrIO server applications.
 * @return
 */
QSettings *
WBIOServerCommon::getSettings()
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
QString
WBIOServerCommon::getDBLocation()
{
    QString dbLocation;
    QSettings *settings = WBIOServerCommon::getSettings();

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
WBIOServerCommon::getClientProcessName(WickrBotClients * client)
{
    QString processName = QString("%1.%2").arg(client->binary).arg(client->name);
    return processName;
}

QStringList
WBIOServerCommon::getAvailableClientApps()
{
    // TODO: Need a way to register possible WickrIO Bots. for now they are hardcoded
    QString possibleBinaries[] = { "test_botAlpha", "test_botBeta", "test_botOnPrem", "test_botQA", "test_bot",
                                   "conformance_botAlpha", "conformance_botOnPrem" };
    QStringList availableBinaries;

    for (QString binary : possibleBinaries) {
        QString filePath = QString("/usr/bin/%1").arg(binary);
        QFileInfo fi(filePath);
        if (fi.exists()) {
            availableBinaries.append(binary);
        }
    }
    return availableBinaries;
}

bool
WBIOServerCommon::isValidClientApp(const QString& binaryName)
{
    QStringList binaries = getAvailableClientApps();

    for (QString binary : binaries) {
        if (binary == binaryName) {
            return true;
        }
    }
    return false;
}
