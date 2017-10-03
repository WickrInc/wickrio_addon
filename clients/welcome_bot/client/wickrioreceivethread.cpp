#include <QJsonArray>
#include <QJsonDocument>

#include "wickrioreceivethread.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"

#include "common/wickrNotifyList.h"

#include "wickrIOMsgEmailService.h"
#include "wickrIOClientRuntime.h"

#include "createjson.h"

#define WICKRBOT_UPDATE_STATS_SECS      600

WickrIOReceiveThread::WickrIOReceiveThread(OperationData *operation) :
    WickrIOThread(),
    m_operation(operation),
    m_enableSwitchboard(false),
    m_receiving(false),
    m_timerStatsTicker(0),
    m_convoList(NULL)
{
    // Initialize the reponse message list
    m_responseMessagesList = QStringList{
        "Hey there! Thanks for messaging me! I have a few helpful but random tips I can share in response to your messages, "
        "so please bear with me☺ If you have more questions than I have answers, head to Settings > Support in Wickr Me. "
        "Way to go to protect your privacy!",

        "Here is how to find your friends on Wickr Me:\n\n"
        "Go to New Conversations> tap on the contact you want to message if you see them, or start typing their Wickr ID in "
        "the contact field. For group conversations: tap and hold on multiple contacts on Android, or select more than 1 "
        "participants in iOS and desktop.",

        "Here is how to set expiration on your messages:\n\n"
        "Expiration is the max time your message will live. Burn-On-Read (BOR) is how long your message will live once the "
        "recipient(s) has seen it. You can change both by tapping on the (i) next to any conversation name, at the top of "
        "your screen.",

        "Video Verification:\n"
        "If you'd like to be sure you are talking to the right person, you can send them a verification request. Tap on a user's "
        "avatar>then on the key icon. You can read more on why key verification is cool for your privacy on our blog: "
        "https://medium.com/cryptoblog/key-verification-in-secure-messaging-bd93a1bf3d40",

        "How to invite friends to join you on Wickr Me:\n\n"
        "You can invite your friends by going to your Settings > Contacts > then tap Invite in the top right corner of your app. "
        "Chose to invite friends by either sending a text or email from your device. They will need to download the app, and "
        "create a Wickr ID to communicate with you here.\n\n"
        "We never store your device contacts on our servers. All invitations are generated locally on your device, without "
        "sharing any information with us.",

        "Sending files on Wickr Me:\n\n"
        "You can now send photos, videos, and other files via Wickr Me, up to 10 MB. This feature supports collaboration "
        "and maximum data hygiene for you and the contacts you TRUST. If you do not trust the person you’re talking to, do "
        "not open files coming from them or send them photos/files you do not want to be saved. Stay safe!",

        "Verification\n\n"
        "You’ll notice an orange dot around your contacts’ avatars – that means you have not yet verified them.\n\n"
        "You don’t have to, but in case you want to make sure you are talking to the right person, send them a key video "
        "verification request to establish trust between your Wickr Me accounts.\n\n"
        "Check out our blog on this: https://medium.com/cryptoblog/key-verification-in-secure-messaging-bd93a1bf3d40",

        "Passwords\n\n"
        "Important to know: there is no password reset on Wickr Me – we don't know who you are which prevents us from "
        "verifying you to reset your password.\n\n"
        "So please remember your password☺",

        "Client Support\n\n"
        "You can use Wickr Me on mobile or desktop to stay in touch with your friends across all your devices.\n\n"
        "Go to www.me-download.wickr.com to download and install on your other devices.",

        "Privacy\n\n"
        "We built Wickr Me to provide private communications to everyone.\n"
        "We take your privacy & security very seriously, learn more: www.wickr.com/security.\n\n"
        "Source code https://github.com/WickrInc/wickr-crypto-c. FAQ www.wickr.com/faq"

    };
}

WickrIOReceiveThread::~WickrIOReceiveThread()
{
    if (m_convoList != NULL) {
        stopReceiving();
    }
}


void WickrIOReceiveThread::processStarted()
{
    qDebug() << "Started WickrIOReceiveThread";
    initMessageServicesConnections();

    // Login successful, so login to switchboard
    startSwitchboard();

    emit signalProcessStarted();
}


void WickrIOReceiveThread::onTimerAction()
{
    m_timerStatsTicker++;

    // If it is time to output statistics then set the appropriate flag
    if ((m_timerStatsTicker % WICKRBOT_UPDATE_STATS_SECS) == 0) {
        logCounts();
    }
}

void
WickrIOReceiveThread::slotProcessMessage(WickrDBObject *item)
{
    if (processMessage(item)) {
        WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;
        if (msg) {
            msg->dodelete(traceInfo());
//            delete msg;
        }
    }
}

bool
WickrIOReceiveThread::processMessage(WickrDBObject *item)
{
    Q_UNUSED(item)

    if (m_shuttingdown || m_threadState == Idle) {
        makeIdle();
        return false;
    }

    // If not handling inbox messages then get rid of the message and leave
    if (!m_operation->m_handleInbox) {
        return true;
    }

    /*
     * Check if there is a callback defined, for this current client.
     * If there is no callback then we do not need to process the messages.
     */
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        return false;
    }

    m_processing = true;
    WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;

    // If this is an outbox message then return (drop the message)
    if (msg && !msg->isInbox()) {
        qDebug() << "slotProcessMessage: have an outbox message, to drop!";
        m_processing = false;

        WickrCore::WickrOutbox *outbox = (WickrCore::WickrOutbox *)msg;

        if (outbox->isDeleted()) {
            return false;
        } else {
            return true;
        }
    }

    qDebug() << "slotProcessMessage: have a message to be processed!";

    if (msg && msg->isInbox()) {
        QString inboxState;
        bool gotInboxMsg = false;
        m_messagesRecv++;
        // Increment the statistic in the database
        if (m_operation->m_botDB != NULL) {
            m_operation->m_botDB->incStatistic(m_operation->m_client->id, DB_STATID_MSGS_RX, DB_STATDESC_TOTAL, 1);
        }

        WickrCore::WickrInbox *inbox = (WickrCore::WickrInbox *)msg;
        switch( inbox->getInboxState() ) {
        case WICKR_INBOX_NEEDS_DOWNLOAD:
            inboxState = "NEEDS_DOWNLOAD";
            break;
        case WICKR_INBOX_OPENED:
            inboxState = "OPENED";
            gotInboxMsg = true;
            break;
        case WICKR_INBOX_EXPIRED:
            inboxState = "EXPIRED";
            break;
        case WICKR_INBOX_DELETED:
            inboxState = "DELETED";
            break;
        }

        if( gotInboxMsg ) {
            WickrMsgClass mclass = msg->getMsgClass();

            if ( mclass == MsgClass_Text || mclass == MsgClass_File) {
                QList<QString> attachments;
                CreateJsonAction *action = new CreateJsonAction("sendmessage",
                                                                msg->getvGroupID(),
                                                                60,
                                                                responseMessageText(msg->getSenderUserID()),
                                                                attachments,
                                                                true);
                QByteArray json = action->toByteArray();
                delete action;
                QDateTime runTime = QDateTime::currentDateTime();

                db->insertAction(json, runTime, m_operation->m_client->id);
            }
        }
    }

    m_processing = false;
    if (m_shuttingdown) {
        if (!m_processing)
            makeIdle();
    }
    return true;
}

const QString& WickrIOReceiveThread::responseMessageText(const QString& userID)
{
    int max = m_responseMessagesList.length();
    int index;

    if (!userID.isEmpty()) {
        if (m_userResponseIndexMap.contains(userID)) {
            index = m_userResponseIndexMap[userID];
            if (++index >= max) index = 0;
            m_userResponseIndexMap[userID] = index;
        } else {
            m_userResponseIndexMap[userID] = 0;
            index = 0;
        }
    } else {
#if 1
        static int backupindex = 0xFFFF;
        backupindex++;
        if (backupindex >= max) {
            backupindex = 0;
        }
        index = backupindex;
#else
        int index = qrand() % max;
#endif
    }
    return m_responseMessagesList.at(index);
}


void WickrIOReceiveThread::startReceiving()
{
    if (! m_receiving) {
        m_receiving = true;

        m_convoList = WickrCore::WickrConvo::getConvoList();
        attachConvos();
    }
    emit signalReceivingStarted();
}

void WickrIOReceiveThread::stopReceiving()
{
    if (m_receiving) {
        if (m_convoList != NULL) {
            detachConvos();
            m_convoList = NULL;
        }
        m_receiving = false;
    }
    emit signalReceivingEnded();
}

void WickrIOReceiveThread::attachConvos()
{
    if (m_convoList) {
        QList<WickrDBObject *>items = m_convoList->acquireItemList(false);
        foreach(WickrDBObject *item, items) {
            slotConvoAdded(item, false);
        }

        connect(m_convoList, SIGNAL(addedItem(WickrDBObject*)),   this, SLOT(slotConvoAdded(WickrDBObject*)), Qt::QueuedConnection);
        connect(m_convoList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotConvoChanged(WickrDBObject*)), Qt::QueuedConnection);
        connect(m_convoList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoDeleted(WickrDBObject*)), Qt::QueuedConnection);

        m_convoList->releaseAcquire();
    }
}

void WickrIOReceiveThread::detachConvos()
{
    if(m_convoList) {
        disconnect(m_convoList, SIGNAL(addedItem(WickrDBObject*)),   this, SLOT(slotConvoAdded(WickrDBObject*)));
        disconnect(m_convoList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotConvoChanged(WickrDBObject*)));
        disconnect(m_convoList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoDeleted(WickrDBObject*)));
    }
}

void WickrIOReceiveThread::slotConvoAdded(WickrDBObject *item, bool existing)
{
    WickrCore::WickrConvo *convo = static_cast<WickrCore::WickrConvo *>(item);
    if (convo) {
        // TODO: Need to check if this convo is setup already or not
        if (existing) {
            // Check if the convo is in a Key Validation state that needs to be acted on
            if (convo->getKeyVerState() == WickrCore::WickrConvo::KeyVerStateGotVideo) {
                // We have the users video, so verify and send a key verification response
                WickrCore::WickrKeyVerificationMgr *wkvm = WickrCore::WickrRuntime::getKeyVerifyMgr();
                if (wkvm) {
                    wkvm->acceptVerification(convo);
                }
            }
        } else {
            attachConvosMessages(convo->getMessages());
        }
    }
}

void WickrIOReceiveThread::slotConvoChanged(WickrDBObject *item) {
    slotConvoAdded(item, true);
}

void WickrIOReceiveThread::slotConvoDeleted(WickrDBObject *inItem) {
    if(inItem != NULL) {
        WickrCore::WickrConvo *convo = static_cast<WickrCore::WickrConvo *>(inItem);
        detachConvosMessages(convo->getMessages());
    }
}


void WickrIOReceiveThread::attachConvosMessages(WickrNotifyList *msgList)
{
    if (msgList != NULL) {
        QList<WickrDBObject*> toBeDeleted;
        QList<WickrDBObject*> items = msgList->acquireItemList(false);
        foreach(WickrDBObject *item, items) {
            //qDebug() << "**** FILTER " << item;
            if (item != NULL) {
                if (processMessage(item)) {
                    toBeDeleted.append(item);
                }
            }
        }

        connect(msgList, SIGNAL(addedItem  (WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
        connect(msgList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
//        connect(msgList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoMessageDeleted(WickrDBObject*)));

        msgList->releaseAcquire();

        foreach(WickrDBObject *item, toBeDeleted) {
            WickrCore::WickrMessage *msg = (WickrCore::WickrMessage *)item;
            if (msg) {
                msg->dodelete(traceInfo());
    //            delete msg;
            }
        }
    }
}

void WickrIOReceiveThread::detachConvosMessages(WickrNotifyList *msgList)
{
    disconnect(msgList, SIGNAL(addedItem  (WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
    disconnect(msgList, SIGNAL(changedItem(WickrDBObject*)), this, SLOT(slotProcessMessage(WickrDBObject*)));
//    disconnect(msgList, SIGNAL(deletedItem(WickrDBObject*)), this, SLOT(slotConvoMessageDeleted(WickrDBObject*)));
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



void WickrIOReceiveThread::initMessageServicesConnections()
{
}
