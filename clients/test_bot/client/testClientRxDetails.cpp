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
            QJsonObject jsonObject;
            bool processMsg = true;

            WickrMsgClass mclass = msg->getMsgClass();
            if( mclass == MsgClass_Text ) {
                QString txt = msg->getCachedText();
                jsonObject.insert(APIJSON_MESSAGE, txt);
                deleteMsg = true;
            } else if (mclass == MsgClass_File) {
                // File needs to be processed before it can be deleted
                deleteMsg = false;
            } else {
                processMsg = false;
            }

            if (processMsg) {
                // Add the message ID
                jsonObject.insert(APIJSON_MSGID, msg->getSrvMsgID());

                if (inbox->getConvo()->getConvoType() == CONVO_SECURE_ROOM ||
                    inbox->getConvo()->getConvoType() == CONVO_GROUP_CONVO) {
                    jsonObject.insert(APIJSON_VGROUPID, inbox->getConvo()->getVGroupID());
                }
                // Get the sender of this message
                WickrCore::WickrUser *sender = inbox->getSenderUser();
                jsonObject.insert(APIJSON_MSGSENDER, sender->getUserID());

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


                // Message timestamp
                long timestamp = msg->getMsgTimestamp();
                QDateTime msgDate;
                msgDate.setMSecsSinceEpoch((quint64)timestamp * 1000l);
                // get the "hh:mm ap" format based on locale
                QString statusText = msgDate.toString( QLocale::system().dateTimeFormat(QLocale::ShortFormat));
                jsonObject.insert(APIJSON_MSGTIME, statusText);


                QString respondApiText = m_operation->getResponseURL();
                if (!respondApiText.isEmpty()) {
                    jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
                }

                QJsonDocument saveDoc(jsonObject);

                if (mclass != MsgClass_File) {
                    int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(QJsonDocument::Compact), (int)msg->getMsgClass(), 0);
                    WickrIOClientRuntime::cbSvcMessagesPending();
                } else {
#if 0
                    WickrIORxDownloadFile *rxDownload = NULL;

                    // Check if we are already downloading or decrypting this file
                    if (m_activeDownloads.contains(msg->getMsgIDSecure())) {
                        rxDownload = m_activeDownloads.value(msg->getMsgIDSecure(), NULL);
                    } else if (m_activeDownloads.size() == 0){
                        if (!msg->getFileInfo().isEmpty()) {
                            WickrCore::FileInfo fileInfo = msg->getFileInfo().at(0);

                            QString dLoadFileName = fileInfo.fileName();
                            QString realFileName;
                            QString extension;
                            if (!dLoadFileName.isEmpty()) {
                                extension = "_" + dLoadFileName;
                                realFileName = dLoadFileName;
                            } else {
                                if(fileInfo.metaData().mimeType() == "image/png")
                                    extension = ".png";
                                else if (fileInfo.metaData().mimeType() == "image/jpeg")
                                    extension = ".jpeg";
                                else if (fileInfo.metaData().mimeType() == "image/bmp")
                                    extension = ".bmp";
                                else if (fileInfo.metaData().mimeType() == "image/gif")
                                    extension = ".gif";
                            }

                            QDateTime dateTime = QDateTime::currentDateTime();
                            QString dateTimeString = dateTime.toString("yyyyMMddhhmmsszzz");

                            QString dirName = m_operation->attachmentsDir;

                            if (dirName.isEmpty()) {
                                // Save the attachment to the temp dir
                                dirName = QDir::tempPath();
                            }

                            QString attachmentFileName(dirName +
                        #ifdef Q_OS_LINUX
                                    "/" +
                        #endif
                             "attachment_" + dateTimeString + extension);

                            if (realFileName.isEmpty())
                                realFileName = attachmentFileName;
                            rxDownload = new WickrIORxDownloadFile(msg, fileInfo, attachmentFileName, realFileName);
                            m_activeDownloads.insert(msg->getMsgIDSecure(), rxDownload);
                        } else {
                            qDebug() << "FileInfo is not set!";
                            deleteMsg = true;
                        }
                    }

                    if (rxDownload != NULL) {
                        WickrCore::WickrCloudTransferMgr *cloudMgr = WickrCore::WickrRuntime::getCloudMgr();
                        if (cloudMgr) {
                            if (! rxDownload->m_downloaded) {
                                if (rxDownload->m_downloading) {
                                    // Check if the download has completed
                                    QFileInfo f(rxDownload->m_attachmentFileName);
                                    if (f.exists()) {
                                        rxDownload->m_downloaded = true;
                                        rxDownload->m_downloading = false;
                                    }
                                } else {
                                    rxDownload->m_downloading = true;
                                    cloudMgr->downloadFile(msg->getConvo(), rxDownload->m_attachmentFileName, rxDownload->m_fileInfo);
                                }
                            }
                        } else {

                        }

                        // If done downloading and decrypting then pass off
                        if (rxDownload != NULL && rxDownload->m_downloaded) {
                            int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(), (int)msg->getMsgClass(), 1);
                            db->insertAttachment(msgID, rxDownload->m_attachmentFileName, rxDownload->m_realFileName);
                            m_activeDownloads.remove(msg->getMsgIDSecure());
                            deleteMsg = true;
                        }
                    }
#endif
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
