#ifndef WICKRIORECEIVETHREAD_H
#define WICKRIORECEIVETHREAD_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "wickrIOThread.h"
#include "operationdata.h"

#include "services/wickrMessageService.h"
#include "services/wickrSwitchboardService.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"

#include "common/wickrMessageMgr.h"
#include "messaging/wickrGroupControl.h"
#include "wickrIOFileDownloadService.h"

/**
 * @brief The WickrIOReceiverMgr class
 * This class handles the messages received from the Message manager
 */
class WickrIOReceiverMgr : public WickrMessageMgr
{
public:
    WickrIOReceiverMgr();

    bool dispatch(WickrCore::WickrMessage *msg);

    int messagesReceived() {
        int msgs = m_messagesRecv;
        m_messagesRecv = 0;
        return msgs;
    }
    int messagesFailed() {
        int failed = m_messagesRecvFailed;
        m_messagesRecvFailed = 0;
        return failed;
    }

private:
    OperationData *m_operation;

    int m_messagesRecv;
    int m_messagesDropped;
    int m_messagesRecvFailed;

};


/**
 * @brief The WickrIOReceiveThread class
 * This class controls the receive thread, which controls the WickrIO Message
 * Manager object.
 */
class WickrIOReceiveThread : public WickrIOThread
{
    Q_OBJECT
public:
    WickrIOReceiveThread();
    ~WickrIOReceiveThread();

    Q_INVOKABLE void slotStartReceiving();
    Q_INVOKABLE void slotStopReceiving();

private:
    WickrIOReceiverMgr m_msgReceiver;

    OperationData *m_operation;
    bool m_enableSwitchboard;
    bool m_receiving;
    int m_timerStatsTicker;

    QString getAttachmentFile(const QByteArray &data, QString extension);

    void startSwitchboard();
    void stopSwitchboard();

protected:
    void processStarted();
    void onTimerAction();

private slots:
signals:
    void signalProcessStarted();
    void signalReceivingStarted();
    void signalReceivingEnded();

    void signalMessageCheck(WickrApplicationState appContext);

};

#endif // WICKRIORECEIVETHREAD_H
