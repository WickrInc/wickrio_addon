#ifndef WICKRIOCLIENTMAIN_H
#define WICKRIOCLIENTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickriodatabase.h"
#include "wickrIOJson.h"
#include "wickrbotlog.h"
#include "operationdata.h"
#include "wickrIOIPCService.h"
#include "user/wickrUser.h"
#include "services/wickrTaskService.h"

#include "wickrIOConvoHdlr.h"
#include "wickrIOClientLoginHdlr.h"
#include "wickrIOActionService.h"
#include "wickrIORxService.h"
#include "wickrIOEventService.h"

#define WICKRBOT WickrIOClientMain::theBot

#define WICKRBOT_NEXTMSG_TIME           86400       // Number of seconds to wait before reponding to received message

#define WICKRBOT_CONFIG_USERNAME    "wickruser"
#define WICKRBOT_CONFIG_PASSWORD    "wickrpassword"
#define WICKRBOT_CONFIG_DORECEIVE   "doreceive"
#define WICKRBOT_CONFIG_SERVERNAME  "servername"

#define WICKRBOT_SERVICE_EVENTSVC   0x01    // Run the Event handler service
#define WICKRBOT_SERVICE_ACTIONSVC  0x02    // Run the Action handler service

class WickrIOClientMain : public QThread
{
    Q_OBJECT

    friend class WickrIOConvoHdlr;
public:
    WickrIOClientMain(OperationData *operation, WickrIORxDetails *rxDetails, unsigned services);
    ~WickrIOClientMain();

    bool startTheClient();

    static WickrIOClientMain *theBot;

    // Parse the settings file (replaces the JSON config file)
    bool parseSettings(QSettings *settings);

    // Function to set connection to the IPC signals
    void setIPC(WickrIOIPCService *ipc);

    WickrIOConvoHdlr m_convoHdlr;

private:
    OperationData           *m_operation;
    WickrIOClientLoginHdlr  m_loginHdlr;
    WickrIOActionService    *m_actionService = nullptr;
    WickrIOEventService     *m_eventService = nullptr;
    unsigned                m_services;

    QTimer timer;
    QString m_serverName;

    long    m_seccount = 0;             // Count off seconds so duration can be determined
    bool    m_durationreached = false;  // true if duration was reached, so exit only once

    WickrIOIPCService *m_wickrIPC = nullptr;

    WickrIORxService    *m_rxService;
    WickrIORxDetails    *m_rxDetails;

    // Timer definitions
    void startTimer()
    {
        connect(&timer, SIGNAL(timeout()), this, SLOT(slotDoTimerWork()));
        timer.start(1000);
    }
    void stopTimer()
    {
        disconnect(&timer, SIGNAL(timeout()), this, SLOT(slotDoTimerWork()));
        if (timer.isActive())
            timer.stop();
    }

    void stopAndExit(int procState);

private slots:
    void slotDoTimerWork();
    void slotLoginSuccess();
    void slotRxProcessStarted();
    void slotRxProcessReceiving();

    void processStarted();
    void stopAndExitSlot();
    void pauseAndExitSlot();

    void slotSwitchboardServiceState(WickrServiceState state, SBSessionStatus sessionStatus, const QString& text);
    void slotMessageServiceState(WickrServiceState state);

    void slotTaskServiceState(WickrServiceState state);

    void slotOnLoginMsgSynchronizationComplete();

    void slotDatabaseLoadDone(WickrDatabaseLoadContext *context);

    void slotAdminUserSuspend(const QString& reason);
    void slotAdminDeviceSuspend();
    void slotSetSuspendError();
    void slotMessageDownloadStatusStart(int msgsToDownload);
    void slotMessageDownloadStatusUpdate(int msgsDownloaded);

    void slotServiceNotLoggedIn();  // Received when Switchboard cannot login for period of time

signals:
    void signalStarted();
    void signalExit();
    void signalLoginSuccess();

    void signalMessageCheck(WickrApplicationState appContext);
};


#endif // WICKRIOCLIENTMAIN_H
