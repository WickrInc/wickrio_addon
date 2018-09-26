#ifndef WICKRIOECLIENTMAIN_H
#define WICKRIOECLIENTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickriodatabase.h"
#include "wickrIOJson.h"
#include "wickrbotlog.h"
#include "wickrIOIPCService.h"
#include "user/wickrUser.h"
#include "services/wickrTaskService.h"
#include "services/wickrSwitchboardService.h"

#include "wickrIOLoginHdlr.h"
#include "wickrIOProvisionHdlr.h"


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
    WickrIOEClientMain(WickrBotClients* client, const QString& invite);
    ~WickrIOEClientMain();

    bool startTheClient();

    static WickrIOEClientMain *theBot;

    static bool loadBootstrapString(const QString& bootstrapStr);

    bool loginSuccess() { return m_loginSuccess; }

private:
    WickrBotClients *m_client;
    QString m_invite;

    WickrIOLoginHdlr m_loginHdlr;

    QTimer timer;
    QString m_serverName;

    bool m_loginSuccess = true;

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
    void slotProvisionPageChanged(WickrIOProvisionHdlr::Page page);
    void slotProvisionFailed(const QString& errorString);

    void slotInitiateLogin();
    void slotLoginSuccess();
    void slotLoginFailure(int returnCode);

    void processStarted();
    void stopAndExitSlot();
    void pauseAndExitSlot();

    void slotSwitchboardServiceState(WickrServiceState state, SBSessionStatus sessionStatus, const QString& text);
    void slotMessageServiceState(WickrServiceState state);

    void slotTaskServiceState(WickrServiceState state);

    void slotOnLoginMsgSynchronizationComplete();

    void slotAdminUserSuspend(SBSessionError code, const QString& reason);
    void slotAdminDeviceSuspend();
    void slotSetSuspendError();
    void slotMessageDownloadStatusStart(int msgsToDownload);
    void slotMessageDownloadStatusUpdate(int msgsDownloaded);

    void slotRegisterOnline(bool online);

signals:
    void signalStarted();
    void signalExit();
    void signalLoginSuccess();
    void signalLoginFailure(int returnCode);

    void signalMessageCheck(WickrApplicationState appContext);
};


#endif // WICKRIOECLIENTMAIN_H
