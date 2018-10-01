#ifndef WICKRIOADDONASYNCSERVICE_H
#define WICKRIOADDONASYNCSERVICE_H

#include <QObject>
#include <QThread>
#include "common/wickrNetworkUtil.h"

#include "operationdata.h"
#include "wickrIOAppSettings.h"
#include "wickrIOServiceBase.h"

#include "nzmqt/nzmqt.hpp"

// Forward declaration
class WickrIOAddonAsyncThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// WickrIOAddonAsync Thread State
enum AddonAsyncThreadState { ADDONASYNC_UNINITIALIZED = 0, // Unitialized
                             ADDONASYNC_STARTED,           // Thread started state, enteres this state only on initial startup.
                             ADDONASYNC_PROCESSING,        // Currently processing messages
                             ADDONASYNC_FINISHED };        // Disconnected from switchboard

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOAddonAsyncService : public WickrIOServiceBase
{
    Q_OBJECT

public:
    explicit WickrIOAddonAsyncService();
    virtual ~WickrIOAddonAsyncService();

    void startScript();

    bool isHealthy();

    void setAsyncMessagesState(bool state);
    bool asyncMessagesState();
    bool sendAsyncMessage(const QString& msg);

    void setAsyncEventsState(bool state);
    bool asyncEventsState();
    bool sendAsyncEvent(const QString& event);


    static QString asyncServiceBaseName;

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    WickrServiceState       m_state;
    QThread                 m_thread;
    WickrIOAddonAsyncThread   *m_cbThread;

    void startThreads();
    void stopThreads();

signals:
    void signalStartScript();

    void signalAsyncMessagesState(bool state);
    void signalSendAsyncMessage(QString msg);
    void signalAsyncMessageSent(bool result);

    void signalAsyncEventsState(bool state);
    void signalSendAsyncEvent(QString event);
    void signalAsyncEventSent(bool result);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The WickrIOAddonAsyncThread class
 */
class WickrIOAddonAsyncThread : public QObject
{
    Q_OBJECT

public:
    static QString jsStringState(AddonAsyncThreadState state);

    WickrIOAddonAsyncThread(QThread *thread, WickrIOAddonAsyncService *swbSvc, QObject *parent=0);
    virtual ~WickrIOAddonAsyncThread();

    bool asyncMessagesState() { return m_processAsyncMessages; }
    bool asyncEventsState() { return m_processAsyncEvents; }

private:
    WickrIOAddonAsyncService    *m_parent;
    AddonAsyncThreadState       m_state;

    bool                    m_jsCallbackInitialized = false;
    QTimer                  m_timer;
    time_t                  m_sentMessageTime;

    // ZeroMQ definitions
    nzmqt::ZMQContext   *m_zctx = nullptr;
    nzmqt::ZMQSocket    *m_async_zsocket = nullptr;

    // Settings for async messages and events
    bool m_processAsyncMessages = false;
    bool m_processAsyncEvents = false;
    bool m_asyncMesgSent = false;           // true if waiting for a response

    bool initJScriptCallback();
    bool stopJScriptCallback();

    bool jScriptStartScript();
    bool jScriptSendMessage();

signals:
    void signalAsyncMessagesState(bool state);
    void signalAsyncMessageSent(bool result);
    void signalAsyncEventsState(bool state);
    void signalAsyncEventSent(bool result);

public slots:
    void slotTimerExpire();
    void slotAsyncResponseReceived(const QList<QByteArray>&);

    void slotStartScript();

    void slotAsyncMessagesState(bool state);
    void slotSendAsyncMessage(QString msg);
    void slotAsyncEventsState(bool state);
    void slotSendAsyncEvent(QString event);

};

#endif // WICKRIOADDONASYNCSERVICE_H
