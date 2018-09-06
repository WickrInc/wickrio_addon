#include "wickrIOCallbackService.h"
#include "wickriodatabase.h"
#include "common/wickrHttpRequest.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOJScriptService.h"

WickrIOCallbackService::WickrIOCallbackService()
    : m_lock(QReadWriteLock::Recursive)
    , m_state(WickrServiceState::SERVICE_UNINITIALIZED)
    , m_cbThread(nullptr)
{
  qRegisterMetaType<WickrServiceState>("WickrServiceState");
  qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

  // Start threads
  startThreads();

  setObjectName("WickrIOCallbackThread");
  qDebug() << "WICKRIOCALLBACK SERVICE: Started.";
  m_state = WickrServiceState::SERVICE_STARTED;
}

/**
* @brief WickrIOCallbackService::~WickrIOCallbackService
* Destructor
*/
WickrIOCallbackService::~WickrIOCallbackService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOCALLBACK SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOCallbackService::startThreads
 * Starts all threads on message service.
 */
void WickrIOCallbackService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new WickrIOCallbackThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOCallbackService::signalMessagesPending,
            m_cbThread, &WickrIOCallbackThread::slotProcessMessages, Qt::QueuedConnection);
    connect(this, &WickrIOCallbackService::signalSetSaveAttachments,
            m_cbThread, &WickrIOCallbackThread::slotSetSaveAttachments, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOCallbackService::stopThreads
 * Stops all threads on WickrIO Callback service.
 */
void WickrIOCallbackService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOCALLBACK THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOCallbackService::messagesPending()
{
    emit signalMessagesPending();
}

void WickrIOCallbackService::setSaveAttachments(bool saveAttachments)
{
    emit signalSetSaveAttachments(saveAttachments);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOCallbackThread::asStringState
 * @param state
 * @return
 */
QString WickrIOCallbackThread::cbStringState(CBThreadState state)
{
    switch(state) {
    case CBThreadState::CB_UNINITIALIZED:   return "Uninitialized";
    case CBThreadState::CB_STARTED:         return "Started";
    case CBThreadState::CB_PROCESSING:      return "Processing";
    case CBThreadState::CB_FINISHED:        return "Finished";
    default:                                return "Unknown";
    }
}

/**
 * @brief WickrIOCallbackThread::WickrIOCallbackThread
 * Constructor
 */
WickrIOCallbackThread::WickrIOCallbackThread(QThread *thread, WickrIOCallbackService *cbSvc)
    : m_parent(cbSvc)
    , m_state(CBThreadState::CB_UNINITIALIZED)
{
    thread->setObjectName("WickrIOCallbackThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    m_state = CBThreadState::CB_STARTED;
}

/**
 * @brief WickrIOCallbackThread::~WickrIOCallbackThread
 * Destructor
 */
WickrIOCallbackThread::~WickrIOCallbackThread() {
    qDebug() << "WICKRIOCALLBACK THREAD: Worker Destroyed.";
}

void
WickrIOCallbackThread::slotSetSaveAttachments(bool saveAttachments)
{
    m_saveAttachments = saveAttachments;
}

bool
WickrIOCallbackThread::isAsyncMessageSet()
{
    /*
     * Start the Integration software if there is any configured
     */
    WickrIOJScriptService *jsSvc = (WickrIOJScriptService*)WickrIOClientRuntime::findService(WickrIOJScriptService::jsServiceBaseName);
    if (!jsSvc)
        return false;

    return jsSvc->asyncMessagesState();
}

bool
WickrIOCallbackThread::sendAsyncMessage()
{
    /*
     * Start the Integration software if there is any configured
     */
    WickrIOJScriptService *jsSvc = (WickrIOJScriptService*)WickrIOClientRuntime::findService(WickrIOJScriptService::jsServiceBaseName);
    if (!jsSvc)
        return false;

    if (!m_asyncMessageSignalSet) {
        connect(jsSvc, &WickrIOJScriptService::signalAsyncMessageSent,
                this, &WickrIOCallbackThread::slotAsyncMessageSent, Qt::QueuedConnection);
        m_asyncMessageSignalSet = true;
    }

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == nullptr) {
        m_operation->log_handler->log("Failed to cast database!");
        return false;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);

    // Make sure the start is less than the number of IDs retrieved
    if (msgIDs.size() == 0) {
        return false;
    } else {
        WickrIOMessage rxMsg;
        if (db->getMessage(msgIDs.at(0), &rxMsg)) {
            m_postedMsgID = rxMsg.id;
            if (jsSvc->sendAsyncMessage(rxMsg.json)) {
                return true;
            }
        }
     }
     return false;

}

void
WickrIOCallbackThread::slotAsyncMessageSent(bool result)
{
    //TODO: Need to have a timeout just in case this signal never comes
    if (m_postedMsgID > 0) {
        WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
        if (db != nullptr) {
            db->deleteMessage(m_postedMsgID, m_saveAttachments);
            m_postedMsgID = 0;
        }
    }

    // If there are no more messages to send then we are done
    if (! sendAsyncMessage()) {
        m_state = CBThreadState::CB_STARTED;
    }

}

void
WickrIOCallbackThread::slotProcessMessages()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == CBThreadState::CB_PROCESSING) {
        return;
    }

    m_state = CBThreadState::CB_PROCESSING;

    if (m_operation == nullptr) {
        m_operation = WickrIOClientRuntime::operationData();
    }

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_state = CBThreadState::CB_STARTED;
    } else {
        WickrIOAppSettings appSetting;

        if (isAsyncMessageSet()) {
            if (!sendAsyncMessage()) {
                m_state = CBThreadState::CB_STARTED;
            }
        } else if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
            QString url = appSetting.value;
            if (! url.isEmpty()) {
                startUrlCallback(url);
            } else {
                m_state = CBThreadState::CB_STARTED;
            }
        } else if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
            WickrIOEmailSettings *email = new WickrIOEmailSettings();
            if (appSetting.getEmail(email)) {
                startEmailCallback(email);
            }
            delete email;
            m_state = CBThreadState::CB_STARTED;
        } else {
            // TODO: Nowhere to send the message, so for now we shall dump it

            m_state = CBThreadState::CB_STARTED;
        }
    }
}


/**********************************************************************************************
 * EMAIL CALLBACK FUNCTIONS
 **********************************************************************************************/

/**
 * @brief WickrIOCallbackThread::startEmailCallback
 * @param email
 */
void
WickrIOCallbackThread::startEmailCallback(WickrIOEmailSettings *email)
{
    SmtpClient::ConnectionType type;
    if (email->type.toLower() == "smtp")
        type = SmtpClient::TcpConnection;
    else if (email->type.toLower() == "ssl")
        type = SmtpClient::SslConnection;
    else if (email->type.toLower() == "tls")
        type = SmtpClient::TlsConnection;

    m_smtp = new SmtpClient(email->server, email->port, type);
    if (m_smtp != NULL) {
        m_smtp->setUser(email->account);
        m_smtp->setPassword(email->password);
        m_smtp->connectToHost();
        m_smtp->login();

        sendMessages(email);

        m_smtp->deleteLater();
    }
}

MimeFile *
WickrIOCallbackThread::getAttachmentFile(const QString &filename)
{
    MimeFile *attachfile  = new MimeFile(new QFile(filename));
    return attachfile;
}

/**
 * @brief WickrIOMsgEmailService::sendMessage
 * This function will attempt to send the currently first message on the list of messages
 * on the m_messages list.  If there are no messages left a false value will be reurned.
 * @return
 */
bool
WickrIOCallbackThread::sendMessages(WickrIOEmailSettings *email)
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_operation->log_handler->log("Failed to cast database!");
        return false;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);
    QList<MimeFile *> attachmentFiles;

    // Make sure the start is less than the number of IDs retrieved
    for (int index : msgIDs) {
        WickrIOMessage rxMsg;
        if (db->getMessage(index, &rxMsg)) {
            MimeMessage message;
            message.setSender(new EmailAddress(email->sender));
            message.addRecipient(new EmailAddress(email->recipient));
            message.setSubject(email->subject);

            MimeText text;
            text.setText(rxMsg.json);
            message.addPart(&text);

            if (rxMsg.hasAttachment) {
                QList<WickrIODBAttachment *> attachments = db->getMsgAttachments(rxMsg.id);
                for (WickrIODBAttachment *attachment : attachments) {
                    MimeFile *attachfile = getAttachmentFile(attachment->m_filename);
                    if (attachfile != NULL) {
                        attachmentFiles.append(attachfile);
                    }
                    delete attachment;
                }
            }

            for (MimeFile* file : attachmentFiles) {
                message.addPart(file);
            }

            m_smtp->sendMail(message);

            // Free the current message memory and delete the message
            db->deleteMessage(rxMsg.id, m_saveAttachments);

            for (MimeFile *file : attachmentFiles) {
                file->deleteLater();
            }

            return true;
        }
    }
}

/**********************************************************************************************
 * CALLBACK FUNCTIONS
 **********************************************************************************************/

void
WickrIOCallbackThread::startUrlCallback(QString url)
{
    m_url = url;
    mgr = new QNetworkAccessManager(this);
    QObject::connect(mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(gotReply(QNetworkReply*)));
    LINK_SERVER_REQ(msgSendCallbackResponse, msgSendCallbackError);

    if (!sendMessage()) {
        CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
        m_state = CBThreadState::CB_STARTED;
    }
}

void
WickrIOCallbackThread::gotReply(QNetworkReply *thereply)
{
    if( !reply || thereply != reply ) {
        //qDebug() << "ignoring this one";
        return;
    }
    reply->deleteLater();

    // Reading attributes of the reply
    // e.g. the HTTP status code
    QVariant statusCodeV = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    // Or the target URL if it was a redirect:
    QVariant redirectionTargetUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    // see CS001432 on how to handle this

    // no error received?
    if (reply->error() == QNetworkReply::NoError)
    {
        // read data from QNetworkReply here
        QByteArray bytes = reply->readAll();
        emit sendReply(reply, bytes);
    }
    // Some http error received
    else
    {
        qDebug() << "gotReply: url=" << reply->url() << " error=" << reply->errorString();
        QString error = tr("Network Connectivity Error %1: Please check your Internet access").arg(statusCodeV.toString());
        emit sendError(reply, error.toLocal8Bit());
    }

    // TODO: Need to handle the processing better
    m_state = CBThreadState::CB_STARTED;
}

/**
 * @brief WickrIOCallbackThread::sendMessage
 * This function will attempt to send the currently first message on the list of messages
 * on the m_messages list.  If there are no messages left a false value will be reurned.
 * @return
 */
bool
WickrIOCallbackThread::sendMessage()
{
    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_operation->log_handler->log("Failed to cast database!");
        return false;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);

    // Make sure the start is less than the number of IDs retrieved
    if (msgIDs.size() == 0) {
        return false;
    } else {
        WickrIOMessage rxMsg;
        if (db->getMessage(msgIDs.at(0), &rxMsg)) {
            m_postedMsgID = rxMsg.id;

            QByteArray postDataSize = QByteArray::number(rxMsg.json.length());
            QUrl serviceURL(m_url);

            QNetworkRequest request(serviceURL);

            request.setRawHeader("User-Agent", "WickrIO v1.1");
            request.setRawHeader("X-Custom-User-Agent", "WickrIO v1.1");
            request.setRawHeader("Content-Type", "application/json");
            request.setRawHeader("Content-Length", postDataSize);

            reply = mgr->post(request, rxMsg.json.toLatin1());
            return true;
        }
     }
     return false;
}

/**
 * @brief WickrIOCallbackThread::msgSendCallbackResponse
 * This slot is called after a callback message was sent successfully.  The associated message
 * will be freed, and then the next message on the list of messages will be sent, if there are
 * any left to send.
 * @param thereply
 * @param bytes
 */
void
WickrIOCallbackThread::msgSendCallbackResponse(QNetworkReply *thereply, QByteArray)
{
    if( thereply != this->reply ) return;

    if (m_postedMsgID > 0) {
        WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
        if (db == NULL) {
            CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
        } else {
            db->deleteMessage(m_postedMsgID, m_saveAttachments);
            m_postedMsgID = 0;
        }
    }

    // If there are no more messages to send then we are done
    if (! sendMessage()) {
        CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
        m_state = CBThreadState::CB_STARTED;
    }
}

void
WickrIOCallbackThread::msgSendCallbackError(QNetworkReply *thereply, QByteArray bytes)
{
    if( thereply != this->reply ) return;

    CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
    m_state = CBThreadState::CB_STARTED;
}

