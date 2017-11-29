#ifndef WICKRIOECLIENTMAIN_H
#define WICKRIOECLIENTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickriodatabase.h"
#include "wickrIOJson.h"
#include "wickrbotlog.h"
#include "operationdata.h"
#include "wickrIOIPCService.h"
#include "wickrioreceivethread.h"
#include "user/wickrUser.h"
#include "services/wickrTaskService.h"

#include "wickrioconvohdlr.h"
#include "wickrIOLoginHdlr.h"
#include "wickrIOActionHdlr.h"

#define WICKRBOT WickrIOEClientMain::theBot

#define WICKRBOT_NEXTMSG_TIME           86400       // Number of seconds to wait before reponding to received message

#define WICKRBOT_CONFIG_USERNAME    "wickruser"
#define WICKRBOT_CONFIG_PASSWORD    "wickrpassword"
#define WICKRBOT_CONFIG_DORECEIVE   "doreceive"
#define WICKRBOT_CONFIG_SERVERNAME  "servername"


class WickrIOEClientMain : public QThread
{
    Q_OBJECT

    friend class WickrIOConvoHdlr;
public:
    WickrIOEClientMain(OperationData *operation);
    ~WickrIOEClientMain();

    bool startTheClient();

    static WickrIOEClientMain *theBot;

    // Parse the settings file (replaces the JSON config file)
    bool parseSettings(QSettings *settings);

    // Function to set connection to the IPC signals
    void setIPC(WickrIOIPCService *ipc);

    WickrIOConvoHdlr m_convoHdlr;

private:
    OperationData *m_operation;
    WickrIOLoginHdlr m_loginHdlr;
    WickrIOActionHdlr m_actionHdlr;

    QTimer timer;
    QString m_serverName;
    long    m_seccount;             // Count off seconds so duration can be determined
    bool    m_durationreached;      // true if duration was reached, so exit only once

    WickrIOIPCService *m_wickrIPC;
    WickrIOReceiveThread *m_rxThread;

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

signals:
    void signalStarted();
    void signalExit();
    void signalLoginSuccess();

    void signalMessageCheck(WickrApplicationState appContext);
};


#endif // WICKRIOECLIENTMAIN_H
