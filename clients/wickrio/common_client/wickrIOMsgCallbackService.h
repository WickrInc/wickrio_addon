#ifndef WICKRIOMSGCLLBACKSERVICE_H
#define WICKRIOMSGCLLBACKSERVICE_H

#include <QMap>
#include "messaging/wickrMessage.h"

// Forward declaration
class WickrIOMsgCallbackThread;
class WickrIOMsgCallbackMessage;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOMsgCallbackService : public QObject
{
    Q_OBJECT
    Q_ENUMS(ServiceState)

public:
    enum ServiceState { UNINITIALIZED = 0,   // Uninitialized
                        STARTED,             // Service started, open for queries/requests.
                        SHUTDOWN };          // Shutdown complete.
    static QString asStringState(ServiceState state);

    WickrIOMsgCallbackService();
    virtual ~WickrIOMsgCallbackService();

    void startThreads();
    void stopThreads();

    // Accessors
    WickrIOMsgCallbackThread *getThread() { return m_cbThread; }

    ServiceState state() const {
        QReadLocker lockGuard(&m_lock);
        return m_state;
    }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    ServiceState                           m_state;
    QThread                                m_threadST;
    WickrIOMsgCallbackThread               *m_cbThread;

signals:
    void signalState(ServiceState state, const QString& text = "");
};


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief The WickrIOMsgCallbackThread class
 */
class WickrIOMsgCallbackThread : public QObject
{
    Q_OBJECT
    Q_ENUMS(ThreadState)

public:
    enum ThreadState { UNINITIALIZED = 0,   // Unitialized
                       STARTED,             // Thread started state, enteres this state only on initial startup.
                       STOPPED };           // Thread stopped state
    static QString asStringState(ThreadState state);

    enum MsgCallbackStatus { MSGCALLBACK_STOPPED,
                             MSGCALLBACK_RUNNING };

    WickrIOMsgCallbackThread(QThread *thread,
                             WickrIOMsgCallbackService *msgCallbackService);
    ~WickrIOMsgCallbackThread();

    // Message Callback control interface
    void sendMessages(QList<WickrIOMsgCallbackMessage *>messages);

private:
    QThread                             m_thread;
    ThreadState                         m_state;
    WickrIOMsgCallbackService           *m_parent;
    QList<WickrIOMsgCallbackMessage *>  m_messages;
    WickrIOMsgCallbackMessage           *m_curMsg;
    QString                             m_url;
    MsgCallbackStatus                   m_status;

    QNetworkAccessManager               *m_mgr;
    QNetworkReply                       *m_reply;

    void cleanup();
    bool processMessagesList();

    void gotReply(QNetworkReply *thereply);
    void slotSslErrors(QNetworkReply *reply, const QList<QSslError>& errors);

public slots:
    void slotSendMessages(QList<WickrIOMsgCallbackMessage *>messages);



    void msgSendCallbackResponse(QNetworkReply*);
    void msgSendCallbackError(QNetworkReply*);

    void sendProgress(qint64 bytesSent, qint64 bytesTotal);
    void finished();

signals:
    void signalSendMessages(QList<WickrIOMsgCallbackMessage *>messages);

};


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The WickrIOMsgCallbackMessage class
 * This class is used to contain the information associated with a message
 * that is to be sent to a specific callback.
 */
class WickrIOMsgCallbackMessage
{
public:
    WickrIOMsgCallbackMessage(const QString& url, const QByteArray& data, WickrCore::WickrMessage *msg);
    ~WickrIOMsgCallbackMessage();

    QStringList             m_urlList;
    QByteArray              m_data;
    WickrCore::WickrMessage *m_msg;
};

#endif // WICKRIOMSGCLLBACKSERVICE_H
