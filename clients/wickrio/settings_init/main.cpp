#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

#include "webserver.h"
#include "wickrbotsettings.h"
#include "wbio_common.h"

#define SETTINGS_SETTINGS_LOCATION "settingslocation"

typedef enum { CHECK_DIR, CHECK_FILE, CHECK_NONE, CHECK_INT } CheckType;

/**
 * @brief getNewValue
 * @param settingsKey
 * @param prompt
 * @return
 */
QString getNewValue(QSettings *pSettings, QString settingsKey, QString prompt, CheckType check=CHECK_NONE)
{
    QString oldSetting = pSettings->value(settingsKey, "").value<QString>();
    QString newSetting("");

    QTextStream s(stdin);

    QString lineInput;

    while (true) {
        if (oldSetting.isEmpty()) {
            qDebug("%s:", qPrintable(prompt));
        } else {
            qDebug("%s (default: %s):", qPrintable(prompt), qPrintable(oldSetting));
        }
        lineInput = s.readLine();
        if (!oldSetting.isEmpty() && lineInput.isEmpty()) {
            newSetting = oldSetting;
            break;
        }
        newSetting = lineInput;

        if (check == CHECK_DIR) {
            QDir settingsDir;
            settingsDir.setPath(lineInput);
            if (settingsDir.exists()) {
                break;
            }
            qWarning("Directory does not exist!");
        } else if (check == CHECK_FILE) {
            if (!lineInput.isEmpty())
                break;
        } else if (check == CHECK_INT) {
            bool ok;
            lineInput.toInt(&ok);
            if (ok)
                break;
        } else {
            break;
        }
    }
    return newSetting;
}

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QSettings *settings;

    QCoreApplication app(argc, argv);
    app.setOrganizationName(WBIO_ORGANIZATION);

#ifdef Q_OS_WIN
    QString settingsFile = QString("HKEY_CURRENT_USER\\Software\\%1\\WickrIO").arg(WBIO_ORGANIZATION);

    settings = new QSettings(settingsFile, QSettings::NativeFormat);

    settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    // deal with the location of the database files
    QString newSettingsLocation(getNewValue(settings, WBSETTINGS_DATABASE_DIRNAME, "Enter database location"));
    settings->setValue(SETTINGS_SETTINGS_LOCATION, newSettingsLocation);
    settings->endGroup();
    settings->sync();
#elif defined(Q_OS_LINUX)
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

    if (argc != 2) {
        qFatal("Usage: %s <install directory>", qPrintable(app.applicationName()));
    }

    QString settingsDir=argv[1];
    QDir installDir(settingsDir);

    // Check if the settings/database directory exists
    if (! installDir.exists()) {
        qFatal("Installation directory does not exist!");
    }

    /*
     * Load some defaults
     */
    QString appName;
    QString settingsFile;

    /*
     * Setup the name of the settings file
     */
    appName = WBIO_CLIENTSERVER_TARGET;
    settingsFile = QString("%1/%2.ini").arg(settingsDir).arg(appName);
    settings = new QSettings(settingsFile, QSettings::NativeFormat);

    /*
     * Deal with the Web Server settings
     */
    WebServer *webServer = new WebServer(settingsDir, settings);

    webServer->m_dbDir = settingsDir;
    webServer->m_appName = appName;

    webServer->saveSettings();

    settings->sync();
#endif
    return 0;
}
