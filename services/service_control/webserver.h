#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QSettings>

#include "operationdata.h"

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QString rootDir, QSettings *settings, QObject *parent = 0);

    bool saveSettings();

signals:

public slots:

public:
    QString m_rootDir;

    QString m_dbDir;
    QString m_url;
    int m_port;

    QSettings *m_settings;
    QString m_appName;
};

#endif // WEBSERVER_H
