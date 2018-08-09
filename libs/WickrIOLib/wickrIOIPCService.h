#ifndef WICKRIOIPCSERVICE_H
#define WICKRIOIPCSERVICE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QString>
#include "common/wickrNetworkUtil.h"

#include "wickrbotdatabase.h"
#include "operationdata.h"

// Forward declaration
class WickrIOIPCRecvThread;
class WickrIOIPCSendThread;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOIPCService : public QObject
{
    Q_OBJECT

    friend class WickrIOIPCRecvThread;
    friend class WickrIOIPCSendThread;

public:
    explicit WickrIOIPCService(const QString& name, bool isClient);
    virtual ~WickrIOIPCService();

    void startIPC(OperationData *operation);
    void shutdown();
    bool isRunning();

    bool sendMessage(const QString& dest, bool isClient, const QString& message);

    // Accessors
    const QString name() { return m_name; }
    bool isClient() { return m_isClient; }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock m_lock;

    QString                 m_name;
    QString                 m_wickrID;
    bool                    m_isClient;

    WickrServiceState       m_state;
    QThread                 m_thread;
    WickrIOIPCRecvThread    *m_ipcRxThread = nullptr;
    WickrIOIPCSendThread    *m_ipcTxThread = nullptr;

    void startThreads();
    void stopThreads();

signals:
    // Signals that are caught by the Threads
    void signalShutdownSend();
    void signalShutdownRecv();
    void signalStartIPC(OperationData *operation);
    void signalSendMessage(const QString& dest, bool isClient, const QString& message);

    // Signals that should be caught by users of this service
    void signalGotStopRequest();
    void signalGotPauseRequest();
    void signalReceivedMessage(QString type, QString value);

    void signalMessageSent();
    void signalMessageSendFailure();

public slots:
};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOIPCRecvThread : public QThread
{
    Q_OBJECT

public:
    explicit WickrIOIPCRecvThread(QThread *thread, WickrIOIPCService *svc);
    virtual ~WickrIOIPCRecvThread();

    void shutdown();

    bool ipcCheck() { return m_ipcCheck; }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOIPCService   *m_parent = nullptr;

    OperationData       *m_operation = nullptr;
    bool                m_ipcCheck = false;

    // ZeroMQ definitions
#ifdef NZMQT_H
    nzmqt::ZMQContext   *m_zctx = nullptr;
    nzmqt::ZMQSocket    *m_zsocket = nullptr;
    QMap<QString, nzmqt::ZMQSocket *>   m_txMap;
#else
    void                *m_zctx = nullptr;
    void                *m_zsocket = nullptr;
    QMap<QString, void *>   m_txMap;
#endif

signals:
    void signalNotRunning();

    void signalGotStopRequest();
    void signalGotPauseRequest();
    void signalReceivedMessage(QString type, QString value);

public slots:
    void slotShutdown();
    void slotStartIPC(OperationData *operation);
    void slotMessageReceived(const QList<QByteArray>& messages);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOIPCSendThread : public QThread
{
    Q_OBJECT

public:
    explicit WickrIOIPCSendThread(QThread *thread, WickrIOIPCService *svc);
    virtual ~WickrIOIPCSendThread();

    void shutdown();

    bool ipcCheck() { return m_ipcCheck; }

private:
    // General purpose thread lock used for common threaded related queries/updates(hence ReadWrite).
    // NOTE: Other required service specific locks should be defined and managed by the specialized services.
    mutable QReadWriteLock      m_lock;

    WickrIOIPCService   *m_parent = nullptr;

    OperationData       *m_operation = nullptr;
    bool                m_ipcCheck = false;

    // ZeroMQ definitions
#ifdef NZMQT_H
    nzmqt::ZMQContext   *m_zctx = nullptr;
    QMap<QString, nzmqt::ZMQSocket *>   m_txMap;
#else
    void                *m_zctx = nullptr;
    QMap<QString, void *>   m_txMap;
#endif

#ifdef NZMQT_H
    nzmqt::ZMQSocket * createSendSocket(const QString& dest, bool isClient);
#else
    void *createSendSocket(const QString& dest, bool isClient);
#endif

signals:
    void signalNotRunning();
    void signalRequestSent();
    void signalRequestFailed();

public slots:
    void slotShutdown();
    void slotStartIPC(OperationData *operation);
    void slotSendMessage(const QString& dest, bool isClient, const QString& message);
    void slotMessageReceived(const QList<QByteArray>& messages);

};

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

class WickrIOIPCCommands
{
public:
    explicit WickrIOIPCCommands() {}

    static QString getBotInfoString(const QString& sendingProc, const QString& name, const QString& procName, const QString& password);
    static QMap<QString,QString> parseBotInfoValue(const QString& value);

    static QString getPasswordString(const QString& sendingProc, const QString& password);
    static QMap<QString,QString> parsePasswordValue(const QString& value);
};

#endif // WICKRIOIPCSERVICE_H
