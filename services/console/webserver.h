#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QSettings>

#include "operationdata.h"

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QString dbDir, QSettings *settings, QObject *parent = 0);

    void getSettings();
    bool saveSettings();
    bool start();

private:
    QString settingsFile();
    QString executableFile();

signals:

public slots:

public:
    QString m_dbDir;
    QSettings *m_settings;
    QString m_appName;

    QString m_logFileName;
    QString m_outFileName;
    QString m_attachmentDirName;
};

#endif // WEBSERVER_H
