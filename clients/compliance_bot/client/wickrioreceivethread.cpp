#include <QJsonArray>
#include <QJsonDocument>

#include "wickrioreceivethread.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"
#include "user/wickrKeyVerificationMessage.h"
#include "calling/wickraudiovideocontrolmessage.h"

#include "common/wickrNotifyList.h"

#include "wickrIOClientRuntime.h"

#include "common/wickrRuntime.h"

#include "wickrIOProcessInbox.h"

#define WICKRBOT_UPDATE_STATS_SECS      3600

WickrIOReceiveThread::WickrIOReceiveThread() :
    WickrIOThread(),
    m_enableSwitchboard(false),
    m_receiving(false),
    m_timerStatsTicker(0)
{
    m_operation = WickrIOClientRuntime::operationData();
}

WickrIOReceiveThread::~WickrIOReceiveThread()
{
    slotStopReceiving();
}

/**
 * @brief WickrIOReceiveThread::onTimerAction
 * This function is called when a timer action goes off.  This should happen
 * one time per second.  Currently this will log statistics.
 */
void WickrIOReceiveThread::onTimerAction()
{
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRBOT_UPDATE_STATS_SECS) == 0) {
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            int msgs = m_msgReceiver.messagesReceived();
            int fails = m_msgReceiver.messagesFailed();
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_MSGS_RX, DB_STATDESC_TOTAL, msgs);

            QString statsMsg = QString("Statistics:\n  Messages received %1\n  Messages failed %2\n")
                    .arg(msgs).arg(fails);
            m_operation->log_handler->log(statsMsg);
        }
    }
}

/**
 * @brief WickrIOReceiveThread::processStarted
 * This function is called when the receive thread is started.  Initialize things
 * and be prepared to start receiving messages.
 */
void WickrIOReceiveThread::processStarted()
{
    qDebug() << "Started WickrIOReceiveThread";

    // Login successful, so login to switchboard
    startSwitchboard();

    emit signalProcessStarted();
}

/**
 * @brief WickrIOReceiveThread::slotStartReceiving
 * The main thread will call this to start receiving messages
 */
void
WickrIOReceiveThread::slotStartReceiving()
{
    // Make sure we are not already receiving
    if (! m_receiving) {
        m_receiving = true;

        // hook in to receive messages, let the show begin!
        WickrCore::WickrRuntime::registerMessageManager(&m_msgReceiver);
    }
    emit signalReceivingStarted();
}

/**
 * @brief WickrIOReceiveThread::slotStopReceiving
 * The main thread will call this to start receiving messages
 */
void
WickrIOReceiveThread::slotStopReceiving()
{
    if (m_receiving) {
        m_receiving = false;

        // hook in to receive messages, let the show begin!
        WickrCore::WickrRuntime::registerMessageManager(nullptr);
    }
    emit signalReceivingEnded();
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOReceiverMgr::WickrIOReceiverMgr
 * Constructure for the WickrIO compliance bot message receiver
 */
WickrIOReceiverMgr::WickrIOReceiverMgr() :
    m_messagesRecv(0),
    m_messagesDropped(0),
    m_messagesRecvFailed(0)
{
    m_operation = WickrIOClientRuntime::operationData();
}

/**
 * @brief WickrIOReceiverMgr::dispatch
 * This is the callback to receive messages
 * Returning true will mean this process is responsible to delete the message
 * @param msg
 * @return true if still using the msg, false if done with it
 */
bool WickrIOReceiverMgr::dispatch(WickrCore::WickrMessage *msg)
{
    bool extendProcessing = false;
    bool failedProcessing = false;

    // Do not handle outbox sync messages
    if (msg->isSyncedOutboxConversion()) {
        return false;
    }

    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_messagesDropped++;
//        msg->doDelete();
//        msg->release();
        return false;
    }

    m_messagesRecv++;

    QJsonObject jsonObject;

    /*
     * Insert common things into the JSON, that is found in all messages
     */
    if (!WickrIOProcessInbox::processCommonFields(jsonObject, msg)) {
        failedProcessing = true;
    }


    if (msg->getMsgBody().targetusers_size()) {
        const QString receiver = QString::fromStdString(msg->getMsgBody().targetusers(0));
        if (!receiver.isEmpty())
            jsonObject.insert(APIJSON_MSGRECEIVER, receiver);
    }


#if 0
    // NOT SUPPORTED RIGHT NOW
    QString respondApiText = m_operation->getResponseURL();
    if (!respondApiText.isEmpty()) {
        jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
    }
#endif

    /*
     * Insert message specific stuff
     */
    WickrCore::WickrInbox *inbox = static_cast<WickrCore::WickrInbox*>(msg);

    WickrMsgClass mclass = msg->getMsgClass();

    if (mclass == MsgClass_Text) {
        QString txt = msg->getCachedText();
        jsonObject.insert(APIJSON_MESSAGE, txt);
    } else if (mclass == MsgClass_File) {
        if (!WickrIOProcessInbox::processFileMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        } else {
            // For file messages the content is sent to another thread
            extendProcessing = true;
        }
    } else if (mclass == MsgClass_KeyVerification) {
        if (!WickrIOProcessInbox::processKeyVerificationMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        }
    } else if (mclass == MsgClass_Control) {
        if (!WickrIOProcessInbox::processControlMsg(jsonObject,  inbox)) {
            //TODO: what to do if this returns false
            failedProcessing = true;
        }
    } else if (mclass == MsgClass_Call) {
        if (!WickrIOProcessInbox::processCallingMsg(jsonObject, inbox)) {
            failedProcessing = true;
        }
    }


    if (! extendProcessing && ! failedProcessing) {
        QJsonDocument saveDoc(jsonObject);

        int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(QJsonDocument::Compact), (int)msg->getMsgClass(), 0);
        WickrIOClientRuntime::cbSvcMessagesPending();
    }

    return extendProcessing;
}

void WickrIOReceiveThread::startSwitchboard()
{
    // Update switchboard login credentials (login is performed only if not already logged in)
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin(),
                                         true);
}

void WickrIOReceiveThread::stopSwitchboard()
{
    WickrCore::WickrRuntime::swbSvcLogout();
}
