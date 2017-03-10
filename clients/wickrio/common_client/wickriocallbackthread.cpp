#include "wickriocallbackthread.h"
#include "wickriodatabase.h"
#include "wickrioapi.h"

#include "wickrIOMsgEmailService.h"
#include "common/wickrHttpRequest.h"

WickrIOCallbackThread::WickrIOCallbackThread(OperationData *operation) :
    WickrIOThread(),
    m_operation(operation)
{
}

WickrIOCallbackThread::~WickrIOCallbackThread()
{

}


void WickrIOCallbackThread::processStarted()
{
    qDebug() << "Started WickrIOCallbackThread";
}


void WickrIOCallbackThread::onTimerAction()
{
    m_processing = true;

    WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
    if (db == NULL) {
        m_processing = false;
    } else {
        WickrIOAppSettings appSetting;

        if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
            QString url = appSetting.value;
            if (! url.isEmpty()) {
                startUrlCallback(url);
            } else {
                m_processing = false;
            }
        } else if (db->getAppSetting(m_operation->m_client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
            WickrIOEmailSettings *email = new WickrIOEmailSettings();
            if (appSetting.getEmail(email)) {
                startEmailCallback(email);
            }
            delete email;
            m_processing = false;
        } else {
            m_processing = false;
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
        m_operation->log("Failed to cast database!");
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
                QList<WickrIODBAttachment *> attachments = db->getAttachments(rxMsg.id);
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
            db->deleteMessage(rxMsg.id);

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

    if (sendMessage()) {
    } else {
        CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
        m_processing = true;
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
        m_operation->log("Failed to cast database!");
        return false;
    }

    QList<int> msgIDs = db->getMessageIDs(m_operation->m_client->id);

    // Make sure the start is less than the number of IDs retrieved
    if (msgIDs.size() == 0) {
        return false;
    } else {
        WickrIOMessage rxMsg;
        if (db->getMessage(msgIDs.at(0), &rxMsg)) {
            postedMsgID = rxMsg.id;

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

    if (postedMsgID > 0) {
        WickrIOClientDatabase *db = static_cast<WickrIOClientDatabase *>(m_operation->m_botDB);
        if (db == NULL) {
            CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
            m_processing = true;
        } else {
            db->deleteMessage(postedMsgID);
            postedMsgID = 0;
        }
    }

    // If there are no more messages to send then we are done
    if (! sendMessage()) {
        CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
        m_processing = true;
    }
}

void
WickrIOCallbackThread::msgSendCallbackError(QNetworkReply *thereply, QByteArray bytes)
{
    if( thereply != this->reply ) return;

    CLEANUP_SERVER_REQ(msgSendCallbackResponse,msgSendCallbackError);
    m_processing = true;
}

