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

#define WICKRBOT_UPDATE_STATS_SECS      600

WickrIOReceiveThread::WickrIOReceiveThread(OperationData *operation) :
    WickrIOThread(),
    m_operation(operation),
    m_enableSwitchboard(false),
    m_receiving(false),
    m_timerStatsTicker(0),
    m_convoList(NULL)
{
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
            msg->dodelete();
//            delete msg;
        }
    }
}

bool
WickrIOReceiveThread::processMessage(WickrDBObject *item)
{
    Q_UNUSED(item)
    bool deleteMsg = false;

    if (m_shuttingdown || m_threadState == Idle) {
        makeIdle();
        return false;
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
                if (inbox->getConvo()->getConvoType() == CONVO_SECURE_ROOM) {
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


                OperationData *pOperation = WickrIOClientRuntime::operationData();
                QString respondApiText = pOperation->getResponseURL();
                if (!respondApiText.isEmpty()) {
                    jsonObject.insert(APIJSON_RESPOND_API, respondApiText);
                }

                QJsonDocument saveDoc(jsonObject);

                if (mclass != MsgClass_File) {
                    int msgID = db->insertMessage(msg->getMsgTimestamp(), m_operation->m_client->id, saveDoc.toJson(), (int)msg->getMsgClass(), 0);
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

    m_processing = false;
    if (m_shuttingdown) {
        if (!m_processing)
            makeIdle();
    }
    return deleteMsg;
}

QString
WickrIOReceiveThread::getAttachmentFile(const QByteArray &data, QString extension)
{
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
     "attachment_" + dateTimeString + "." + extension);

    QFile tempFile(attachmentFileName);
    tempFile.open(QIODevice::WriteOnly);
    tempFile.write(data);
    tempFile.close();
    return attachmentFileName;
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
                msg->dodelete();
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
