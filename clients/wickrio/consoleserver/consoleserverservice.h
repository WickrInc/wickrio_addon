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
    void usage();
    bool configureService();

private:
    OperationData *m_operation;
    QTimer *m_processTimer;
    QString m_vendorName;
    bool m_isConfigured;

    QString m_appNm;
    QSettings *m_settings;
    HttpListener *m_listener;

private slots:
    void slotTimeoutProcess();
};

#endif // WICKRIOCONSOLESERVERSERVICE_H
