#ifndef WICKRIOECLIENTMAIN_H
#define WICKRIOECLIENTMAIN_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>

#include "wickriodatabase.h"
#include "wickriojson.h"
#include "wickrbotlog.h"
#include "operationdata.h"
#include "wickrioipc.h"
#include "wickrioreceivethread.h"
#include "user/wickrUser.h"
#include "services/wickrTaskService.h"

#include "wickrioconvohdlr.h"
#include "wickrIOLoginHdlr.h"

#define WICKRBOT WickrIOEClientMain::theBot

#define WICKRBOT_NEXTMSG_TIME           86400       // Number of seconds to wait before reponding to received message

#define WICKRBOT_CONFIG_USERNAME    "wickruser"
#define WICKRBOT_CONFIG_PASSWORD    "wickrpassword"
#define WICKRBOT_CONFIG_DORECEIVE   "doreceive"
#define WICKRBOT_CONFIG_SERVERNAME  "servername"

#define WICKRBOT_UPDATE_PROCESS_SECS    60

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

    QString getPassword();

    // Function to set connection to the IPC signals
    void setIPC(WickrBotMainIPC *ipc);

    WickrIOConvoHdlr m_convoHdlr;

    QString m_username;
    QString m_password;

    bool loadBootstrapFile();
    bool loadBootstrapString(const QString& bootstrapStr);

private:
    OperationData *m_operation;
    WickrIOLoginHdlr m_loginHdlr;

    QTimer timer;
    int m_timerStatsTicker;
    QString m_serverName;

    WickrBotIPC             m_txIPC;
    WickrBotMainIPC         *m_rxIPC;
    WickrIOReceiveThread    *m_rxThread;

    bool    m_waitingForPassword;

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

    bool sendConsoleMsg(const QString& cmd, const QString& value);

private slots:
    void slotDoTimerWork();
    void slotLoginSuccess(QString userSigningKey);
    void slotRxProcessStarted();
    void slotRxProcessReceiving();

    void processStarted();
    void stopAndExitSlot();
    void pauseAndExitSlot();
    void slotReceivedMessage(QString type, QString value);

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
