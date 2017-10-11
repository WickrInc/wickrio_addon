#ifndef WICKRIOACTIONHDLR_H
#define WICKRIOACTIONHDLR_H

#include <QObject>
#include <QString>

#include "user/wickrUser.h"
#include "common/wickrRequests.h"

#include "wickriodatabase.h"
#include "wickriojson.h"
#include "operationdata.h"
#include "wickriomessagecounter.h"

#define WICKRBOT_INVALID_ID     -1

#define WICKRBOT_UPDATE_STATS_SECS      600

class WickrIOActionHdlr : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOActionHdlr(OperationData *operation);
    ~WickrIOActionHdlr();

    bool isProcessingActions() { return m_processAction; }
    bool isProcessingRcvMsgs() { return m_processRcvMsgs; }
    bool isProcessingCleanUp() { return m_processCleanUp; }
    bool isDelayedRcvOrClean() { return m_delayedRcvOrClean; }
    void setShutowntime(bool value) { m_shutdownTime = value; }
    void setShuttingDown(bool value) { m_shuttingdown = value; }
    bool isShuttingDown() { return m_shuttingdown; }

private:
    OperationData *m_operation;
    bool m_processAction;           // True when processing actions to send messages
    bool m_processRcvMsgs;          // True when processing incoming messages
    bool m_processCleanUp;          // True when db cleanup is processing
    bool m_delayedRcvOrClean;       // Set when a Recive or cleanup is delayed
    WickrBotMessageCounter m_appCounter;
    int m_backoff;
    int m_timerStatsTicker;
    bool m_outputStats;
    bool m_shutdownTime;
    bool m_shuttingdown;

    void processAction(WickrBotJson *jsonHandler, int actionID);
    bool processActionSendMessage(WickrBotJson *jsonHandler, int actionID);
    void sendMessageValidateUserUpdate();
    void sendMessageValidateUserSearch();
    void create1To1ConvoStart(const QString& member);

    void sendMessageToConvo(WickrCore::WickrConvo *convo);

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
        if (m_messagesSent > 0) {
            m_operation->log("Messages sent", m_messagesSent);
            m_messagesSent = 0;
        }
        if (m_messagesFailed > 0) {
            m_operation->log("Messages failed", m_messagesFailed);
            m_messagesFailed = 0;
        }
    }

    void cleanUpDatabase();
    void sendMessageTo1To1(WickrCore::WickrConvo *convo);

    void cleanUpConvoList();

signals:
    void signalSendMessageDoneGetUsers();
    void signalStartProcessDatabase(int actionID);
    void signalExit();

private slots:
    // Message Send slots
    void slotMessageDone(WickrSendContext *context);

    void processDatabase(int deleteID);
    void slotSendMessagePostGetUsers();

    void slotValidateUserUpdateDone(WickrUserValidateUpdate *context);
    void slotValidateUserCheckDone(WickrUserValidateSearch *context);
    void slotSendFileStatusChange(const QString& uuid, const QString& status, float progress, const QString& finalFileName);

public slots:
    void doTimerWork();

};

#endif // WICKRIOACTIONHDLR_H
