#ifndef WICKRIOACTIONSERVICE_H
#define WICKRIOACTIONSERVICE_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>

#include "common/wickrNetworkUtil.h"
#include "wickrIOServiceBase.h"

#include "user/wickrUser.h"
#include "common/wickrRequests.h"

#include "wickriodatabase.h"
#include "wickrIOJson.h"
#include "operationdata.h"
#include "wickrIOMessageCounter.h"


// Forward declaration
class WickrIOActionThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOActionService : public WickrIOServiceBase
{
    Q_OBJECT
public:
    explicit WickrIOActionService(OperationData *operation);
    virtual ~WickrIOActionService();

    void shutdown();
    bool isHealthy();

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;
    WickrIOActionThread    *m_ahThread;
    qint64                  m_processActionStarted=0;

    void startThreads(OperationData *operation);
    void stopThreads();

signals:
    void signalShutdown();

public slots:
    void slotProcessAction(bool state);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#define WICKRIO_AH_INVALID_ID           -1
#define WICKRIO_AH_UPDATE_STATS_SECS    600
#define WICKRIO_AH_UPDATE_PROCESS_SECS  1

class WickrIOActionThread : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOActionThread(QThread *thread, WickrIOActionService *svc, OperationData *operation);
    ~WickrIOActionThread();

    bool isProcessingRcvMsgs() { return m_processRcvMsgs; }
    bool isProcessingCleanUp() { return m_processCleanUp; }
    bool isDelayedRcvOrClean() { return m_delayedRcvOrClean; }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOActionService    *m_parent;
    bool                    m_running;
    OperationData           *m_operation;

    QTimer                  m_timer;

    void doStatusUpdate(int state, bool force);


    bool m_processActionState=false;      // True when processing actions to send messages
    bool m_processRcvMsgs=false;          // True when processing incoming messages
    bool m_processCleanUp=false;          // True when db cleanup is processing
    bool m_delayedRcvOrClean=false;       // Set when a Recive or cleanup is delayed
    WickrIOMessageCounter m_appCounter;
    int m_backoff=0;
    int m_timerStatsTicker=0;
    bool m_outputStats=false;
    bool m_shuttingdown=false;
    int  m_shutdownCountDown;

    bool processActionSendMessage(WickrBotJson *jsonHandler, int actionID);
    void sendMessageValidateUserUpdate();
    void sendMessageValidateUserSearch();
    void create1To1ConvoStart(const QString& member);

    bool sendMessageToConvo(WickrCore::WickrConvo *convo);

    bool sendFile(WickrCore::WickrConvo *targetConvo, const QList<QString> files, const QString& comments);

    // Message sending definitions
    QList<WickrCore::WickrUser *> m_wickrUsers;
    WickrBotJson *m_jsonHandler;
    QList<QString> m_userNames;

    int m_curActionID;
    int m_messagesSent;
    int m_messagesFailed;

    void initCounts()
    {
        m_messagesSent = 0;
        m_messagesFailed = 0;
    }

    void logCounts()
    {
        int msgs = m_messagesSent;    m_messagesSent = 0;
        int fails = m_messagesFailed; m_messagesFailed = 0;

        QString statsMsg = QString("Tx Statistics:\n  Messages sent %1\n  Messages failed %2\n")
                .arg(msgs).arg(fails);
        m_operation->log_handler->log(statsMsg);
        m_operation->postEvent(statsMsg, false);

    }

    void cleanUpDatabase();
    bool sendMessageTo1To1(WickrCore::WickrConvo *convo);

    void cleanUpConvoList();

    void setProcessAction(bool state);
    bool processActionState() { return m_processActionState; }

signals:
    void signalSendMessageDoneGetUsers();
    void signalStartProcessDatabase(int actionID);
    void signalExit();
    void signalNotRunning();
    void signalProcessAction(bool state);

private slots:
    // Message Send slots
    void slotMessageDone(WickrSendContext *context);

    void processDatabase(int deleteID);
    void slotSendMessagePostGetUsers();

    void slotValidateUserUpdateDone(WickrUserValidateUpdate *context);
    void slotValidateUserCheckDone(WickrUserValidateSearch *context);
    void slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName);

public slots:
    void slotTimerExpire();
    void slotShutdown();
};

#endif // WICKRIOACTIONSERVICE_H
