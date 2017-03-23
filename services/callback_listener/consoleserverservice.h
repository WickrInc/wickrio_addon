#ifndef WICKRIOCONSOLESERVERSERVICE_H
#define WICKRIOCONSOLESERVERSERVICE_H

#include <QObject>
#include <QString>
#include <QCoreApplication>
#include <QTimer>

#include <qtservice/qtservice.h>
#include <operationdata.h>
#include <wickrbotipc.h>

#include "cmdhandler.h"

#define UPDATE_STATUS_SECS  60

class WickrIOConsoleServerService : public QObject, public QtService<QCoreApplication>
{
Q_OBJECT

public:
    WickrIOConsoleServerService(int argc, char **argv);
    virtual ~WickrIOConsoleServerService();

protected:
    void start();
    void stop();
    void pause();
    void resume();
    void processCommand(int code);

private:
    bool makeDirectory(QString dirname);
    void usage();
    QString searchConfigFile();
    void updateProcessState(int state);

    bool configureService();

private:
    OperationData *m_operation;
    QTimer *m_processTimer;
    QString m_vendorName;
    int m_statusCntDwn;
    QString m_configFileName;
    bool m_isConfigured;

    QString m_appNm;
    QSettings *m_settings;
    HttpListener *m_listener;

private slots:
    void slotTimeoutProcess();
};

#endif // WICKRIOCONSOLESERVERSERVICE_H
