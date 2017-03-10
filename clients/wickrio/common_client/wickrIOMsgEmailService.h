#ifndef WICKRIOMSGEMAILSERVICE_H
#define WICKRIOMSGEMAILSERVICE_H

#include <QMap>
#include "messaging/wickrMessage.h"
#include "SmtpMime"
#include "wickrioappsettings.h"

// Forward declaration
class WickrIOMsgEmailMessage;
class WickrIOMsgEmailMessages;
class WickrIOMsgEmailThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The WickrIOMsgEmailService class
 * This class is the Message Email Service class
 */
class WickrIOMsgEmailService : public QObject
{
    Q_OBJECT
    Q_ENUMS(ServiceState)

public:
    enum ServiceState { UNINITIALIZED = 0,   // Uninitialized
                        STARTED,             // Service started, open for queries/requests.
                        SHUTDOWN };          // Shutdown complete.
    static QString asStringState(ServiceState state);
    WickrIOMsgEmailService();
    ~WickrIOMsgEmailService();

    void startThreads();
    void stopThreads();

    // Accessors
    WickrIOMsgEmailThread *getMsgEmailThread() { return m_msgEmailThread; }

    ServiceState state() const {
        QReadLocker lockGuard(&m_lock);
        return m_state;
    }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    ServiceState            m_state;
    QThread                 m_threadQT;
    WickrIOMsgEmailThread   *m_msgEmailThread;

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
 * @brief The WickrIOMsgEmailThread class
 */
class WickrIOMsgEmailThread : public QObject
{
    Q_OBJECT
    Q_ENUMS(ThreadState)

public:
    enum ThreadState { UNINITIALIZED = 0,   // Unitialized
                       STARTED,             // Thread started state, enteres this state only on initial startup.
                       STOPPED };           // Thread stopped state
    static QString asStringState(ThreadState state);

    enum MsgEmailServiceStatus { SENDING_STOPPED,
                                 SENDING_RUNNING };

    WickrIOMsgEmailThread(QThread *thread, WickrIOMsgEmailService *msgEmailSvc);
    virtual ~WickrIOMsgEmailThread();

    // control interface
    void sendMessages(WickrIOMsgEmailMessages *messages);

public slots:

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock  m_lock;

    QThread                 m_thread;
    WickrIOMsgEmailService  *m_parent;
    ThreadState             m_state;
    MsgEmailServiceStatus   m_status;
    SmtpClient              *m_smtp;

    QList<WickrIOMsgEmailMessages *>    m_messagesList;
    WickrIOMsgEmailMessages             *m_curMessages;
    WickrIOMsgEmailMessage              *m_curMsg;

    bool processMessageList();
    bool processMessageFromList();
    void cleanup();
    MimeFile *getAttachmentFile(const QByteArray &data, QString extension);

private slots:
    void slotSendMessages(WickrIOMsgEmailMessages *messages);

signals:
    void signalSendMessages(WickrIOMsgEmailMessages *messages);

};



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief The WickrIOMsgEmailMessage class
 * This class is used to contain the information associated with a message
 * that is to be sent to a specific callback.
 */
class WickrIOMsgEmailMessage
{
public:
    QByteArray              m_data;
    WickrCore::WickrMessage *m_msg;
    QList<MimeFile *>       m_attachmentFiles;
    QList<QString>          m_attachmentTempFileNames;
};

/**
 * @brief The WickrIOMsgEmailMessage class
 * This class is used to contain the information associated with a message
 * that is to be sent to a specific callback.
 */
class WickrIOMsgEmailMessages
{
public:
    WickrIOMsgEmailMessages(const WickrIOEmailSettings& emailSettings);
    ~WickrIOMsgEmailMessages();

    void addMessage(WickrIOMsgEmailMessage *);

    WickrIOEmailSettings            m_emailSettings;
    QList<WickrIOMsgEmailMessage *> m_messages;
};




#endif // WICKRIOMSGEMAILSERVICE_H
