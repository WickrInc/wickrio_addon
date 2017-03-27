#ifndef WICKRIOECLIENTMAIN_H
#define WICKRIOECLIENTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickriodatabase.h"
#include "wickriojson.h"
#include "wickrbotlog.h"
#include "wickrioipc.h"
#include "user/wickrUser.h"
#include "services/wickrTaskService.h"

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
    WickrIOEClientMain(const QString& username, const QString& password, const QString& invite);
    ~WickrIOEClientMain();

    bool startTheClient();

    static WickrIOEClientMain *theBot;

    // Parse the settings file (replaces the JSON config file)
    bool parseSettings(QSettings *settings);

private:
    QString m_username;
    QString m_password;
    QString m_invite;

    WickrIOLoginHdlr m_loginHdlr;

    QTimer timer;
    QString m_serverName;

    WickrBotMainIPC *m_wickrIPC;

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

    void slogInitiateLogin();
    void slotLoginSuccess();

    void processStarted();
    void stopAndExitSlot();
    void pauseAndExitSlot();

    void slotDeleteRoom(const QString& vGroupID, bool selfInitiated);
    void slotRemoveFromRoom(const QString& vGroupID);

    void slotSwitchboardServiceState(WickrServiceState state, const QString& text);
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
