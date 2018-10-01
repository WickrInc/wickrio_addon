#include <QJsonArray>
#include <QJsonDocument>

#include "testClientRxDetails.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "messaging/wickrInbox.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "common/wickrRuntime.h"
#include "user/wickrKeyVerificationMgr.h"

#include "common/wickrNotifyList.h"

#include "wickrIOClientRuntime.h"
#include "wickrIOProcessInbox.h"
#include "wickrIOJScriptService.h"

TestClientRxDetails::TestClientRxDetails(OperationData *operation) : WickrIORxDetails(operation)
{
}

TestClientRxDetails::~TestClientRxDetails()
{
}

bool TestClientRxDetails::init()
{
    return true;
}

bool TestClientRxDetails::healthCheck()
{
    return true;
}

bool TestClientRxDetails::processMessage(WickrDBObject *item)
{
    bool deleteMsg = false;

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
        if (m_operation->debug)
            qDebug() << "slotProcessMessage: have an outbox message, to drop!";
        m_processing = false;

        WickrCore::WickrOutbox *outbox = (WickrCore::WickrOutbox *)msg;
        if (outbox->isDeleted()) {
            return false;
        } else {
#if 1
            return false;
#else
            return true;
#endif
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
            bool processMsg = true;

            WickrMsgClass mclass = msg->getMsgClass();
            if( mclass == MsgClass_Text ) {
                deleteMsg = true;
            } else if (mclass == MsgClass_File) {
                // File needs to be processed before it can be deleted
                deleteMsg = false;
            } else {
                processMsg = false;
            }

            if (processMsg) {
                QJsonObject jsonObject;
                bool saveMessage = true;

                if (!WickrIOProcessInbox::processCommonFields(jsonObject, msg)) {
                    saveMessage = false;
                } else {}

                // Setup the users array
                QList<WickrCore::WickrUser *>users = msg->getConvo()->getAllUsers();
                QJsonArray usersArray;

                for (WickrCore::WickrUser *user : users) {
                    QJsonObject userEntry;

                    userEntry.insert(APIJSON_NAME, user->getUserID());
                    usersArray.append(userEntry);
                }
                QJsonObject myUserEntry;
                myUserEntry.insert(APIJSON_NAME, m_operation->m_client->user);
                usersArray.append(myUserEntry);

                jsonObject.insert(APIJSON_USERS, usersArray);


                QString respondApiText = m_operation->getResponseURL();
                if (!respondApiText.isEmpty()) {
                    jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
                }


                if (mclass == MsgClass_Text) {
                    QString txt = msg->getCachedText();
                    jsonObject.insert(APIJSON_MESSAGE, txt);
                } else if (mclass == MsgClass_File) {
                    //
                    saveMessage = false;
                    if (WickrIOProcessInbox::processFileMsg(jsonObject,  inbox)) {
                        // For file messages the content is sent to another thread
                        deleteMsg = true;
                    }
                }


                if (saveMessage) {
                    QJsonDocument saveDoc(jsonObject);

                    int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(QJsonDocument::Compact), (int)msg->getMsgClass(), 0);
                    WickrIOClientRuntime::cbSvcMessagesPending();
                }
            }

            // Message had been processed so remove from the client database
#if 0 // cannot delete msg here, causes a deadlock
            if (deleteMsg)
                msg->dodelete();
#endif
        }
    }
    return deleteMsg;
}
