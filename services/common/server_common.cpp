#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include "wickrIOCommon.h"
#include "server_common.h"
#include "wickrbotsettings.h"

bool WBIOServerCommon::m_initialized = false;
QList<WBIOClientApps *>  WBIOServerCommon::m_botApps;
QStringList              WBIOServerCommon::m_bots;
QStringList              WBIOServerCommon::m_parsers;

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

QString
WBIOServerCommon::getParserProcessName(WickrIOParsers * parser)
{
    qDebug() << "TODO: Implement getParserProcessName()";
    QString processName;
    return processName;
}

void
WBIOServerCommon::initClientApps()
{
    if (!m_initialized) {
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("test_botAlpha",       nullptr,                nullptr,               false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("test_botBeta",        nullptr,                nullptr,               false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("test_botQA",          nullptr,                nullptr,               false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("test_bot",            nullptr,                nullptr,               false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("compliance_botAlpha", "compliance_provAlpha", nullptr,               true));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("compliance_botBeta",  "compliance_provBeta",  nullptr,               true));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("compliance_botQA",    "compliance_provQA",    nullptr,               true));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("compliance_bot",      "compliance_prov",      nullptr,               false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("welcome_botAlpha",    nullptr,                "welcome_parserAlpha", false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("welcome_botBeta",     nullptr,                "welcome_parserBeta",  false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("welcome_botQA",       nullptr,                "welcome_parserQA",    false));
        WBIOServerCommon::m_botApps.append(new WBIOClientApps("welcome_bot",         nullptr,                "welcome_parser",      false));

        for (WBIOClientApps *botapp : WBIOServerCommon::m_botApps) {
            m_bots.append(botapp->bot());
            if (!botapp->m_parserApp.isEmpty()) {
                WBIOServerCommon::m_parsers.append(botapp->m_parserApp);
            }
        }
        m_initialized = true;
    }
}

QStringList
WBIOServerCommon::getAvailableClientApps()
{
    initClientApps();
    QStringList availableBinaries;

    for (QString binary : WBIOServerCommon::m_bots) {
        QString filePath = QString("/usr/bin/%1").arg(binary);
        QFileInfo fi(filePath);
        if (fi.exists()) {
            availableBinaries.append(binary);
        }
    }
    return availableBinaries;
}

QStringList
WBIOServerCommon::getAvailableParserApps()
{
    initClientApps();
    QStringList availableBinaries;

    for (QString binary : WBIOServerCommon::m_parsers) {
        QString filePath = QString("/usr/bin/%1").arg(binary);
        QFileInfo fi(filePath);
        if (fi.exists()) {
            availableBinaries.append(binary);
        }
    }
    return availableBinaries;
}

QString
WBIOServerCommon::getProvisionApp(const QString& clientApp)
{
    // Assuming that the apps are initialized

    for (WBIOClientApps *botapp : WBIOServerCommon::m_botApps) {
        if (botapp->bot() == clientApp) {
            return botapp->provision();
        }
    }
    return nullptr;
}

QString
WBIOServerCommon::getParserApp(const QString& clientApp)
{
    // Assuming that the apps are initialized

    for (WBIOClientApps *botapp : WBIOServerCommon::m_botApps) {
        if (botapp->bot() == clientApp) {
            return botapp->parser();
        }
    }
    return nullptr;
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

bool
WBIOServerCommon::isPasswordRequired(const QString& binaryName)
{
    WBIOServerCommon::initClientApps();

    for (WBIOClientApps *botapp : WBIOServerCommon::m_botApps) {
        if (botapp->bot() == binaryName) {
            return botapp->pwRequired();
        }
    }
    return false;
}
