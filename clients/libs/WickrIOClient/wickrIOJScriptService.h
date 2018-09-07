#ifndef WICKRIOJSCRIPTSERVICE_H
#define WICKRIOJSCRIPTSERVICE_H

#include <QObject>
#include <QThread>
#include "common/wickrNetworkUtil.h"

#include "operationdata.h"
#include "wickrIOAppSettings.h"
#include "wickrIOServiceBase.h"

#include "nzmqt/nzmqt.hpp"

// Forward declaration
class WickrIOJScriptThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// WickrIOJScript Thread State
enum JSThreadState { JS_UNINITIALIZED = 0, // Unitialized
                     JS_STARTED,           // Thread started state, enteres this state only on initial startup.
                     JS_PROCESSING,        // Currently processing messages
                     JS_FINISHED };        // Disconnected from switchboard

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOJScriptService : public WickrIOServiceBase
{
    Q_OBJECT

public:
    explicit WickrIOJScriptService();
    virtual ~WickrIOJScriptService();

    void messagesPending();
    void startScript();

    bool isHealthy();

    bool asyncMessagesState();
    bool sendAsyncMessage(const QString& msg);
    bool asyncEventsState();
    bool sendAsyncEvent(const QString& event);

    static QString jsServiceBaseName;

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    WickrServiceState       m_state;
    QThread                 m_thread;
    WickrIOJScriptThread   *m_cbThread;

    void startThreads();
    void stopThreads();

signals:
    void signalMessagesPending();
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
 * @brief The WickrIOJScriptThread class
 */
class WickrIOJScriptThread : public QObject
{
    Q_OBJECT

public:
    static QString jsStringState(JSThreadState state);

    WickrIOJScriptThread(QThread *thread, WickrIOJScriptService *swbSvc, QObject *parent=0);
    virtual ~WickrIOJScriptThread();

    bool asyncMessagesState() { return m_processAsyncMessages; }
    bool asyncEventsState() { return m_processAsyncEvents; }

private:
    WickrIOJScriptService  *m_parent;
    JSThreadState           m_state;

    bool                    m_jsCallbackInitialized = false;
    QTimer                  m_timer;
    time_t                  m_sentMessageTime;

    // ZeroMQ definitions
    nzmqt::ZMQContext   *m_zctx = nullptr;
    nzmqt::ZMQSocket    *m_zsocket = nullptr;
    nzmqt::ZMQSocket    *m_async_zsocket = nullptr;

    // Settings for async messages and events
    bool m_processAsyncMessages = false;
    bool m_processAsyncEvents = false;
    bool m_asyncMesgSent = false;           // true if waiting for a response

    bool initJScriptCallback();
    bool stopJScriptCallback();

    bool jScriptStartScript();
    bool jScriptSendMessage();

    QString processRequest(const QByteArray& request);

signals:
    void signalAsyncMessagesState(bool state);
    void signalAsyncMessageSent(bool result);
    void signalAsyncEventsState(bool state);
    void signalAsyncEventSent(bool result);

public slots:
    void slotTimerExpire();

    void slotMessageReceived(const QList<QByteArray>&);
    void slotAsyncResponseReceived(const QList<QByteArray>&);

    void slotProcessMessages();
    void slotStartScript();

    void slotSendAsyncMessage(QString msg);
    void slotSendAsyncEvent(QString event);

};

#endif // WICKRIOJSCRIPTSERVICE_H
