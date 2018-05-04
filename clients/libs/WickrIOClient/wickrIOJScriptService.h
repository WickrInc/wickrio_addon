#ifndef WICKRIOJSCRIPTSERVICE_H
#define WICKRIOJSCRIPTSERVICE_H

#include <QObject>
#include <QThread>
#include "common/wickrNetworkUtil.h"

#include "operationdata.h"
#include "wickrIOAppSettings.h"
#include "wickrIOServiceBase.h"

#include "libplatform/libplatform.h"
#include "v8.h"

using namespace v8;

// Forward declaration
class WickrIOJScriptThread;
class QAmqpQueue;
class QAmqpExchange;
class QAmqpClient;

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

public slots:
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

private:
    WickrIOJScriptService  *m_parent;
    JSThreadState           m_state;
    OperationData           *m_operation = nullptr;

    bool                    m_jsCallbackInitialized = false;

    // v8 definitions
    v8::Platform*               m_platform = nullptr;
    v8::Isolate*                m_isolate = nullptr;
    v8::Persistent<v8::Context> m_persistent_context;

    v8::Persistent<v8::Script>  m_compiledScript;
    v8::Local<v8::Value>        m_result;

    bool initJScriptCallback();
    bool stopJScriptCallback();

    bool jScriptStartScript();
    bool jScriptSendMessage();

    QString processRequest(const QByteArray& request);

signals:

public slots:
    void slotProcessMessages();
    void slotStartScript();

    void slotListen();

private slots:
    void slotClientConnected();
    void slotQueueDeclared();
    void slotQosDefined();
    void slotProcessRpcMessage();

private:
    QAmqpClient *m_client = nullptr;
    QAmqpQueue *m_rpcQueue = nullptr;
    QAmqpExchange *m_defaultExchange = nullptr;
};

#endif // WICKRIOJSCRIPTSERVICE_H
