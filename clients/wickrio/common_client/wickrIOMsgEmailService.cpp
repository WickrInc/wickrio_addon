#include "wickrIOMsgEmailService.h"
#include "qbson.h"
#include "wickriotokens.h"

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOMsgEmailService::WickrIOMsgEmailService() :
    m_state(UNINITIALIZED),
    m_msgEmailThread(nullptr)
{
    // Start threads
    startThreads();

    setObjectName("WickrIOMsgEmailService");
    qDebug() << "WickrIO Msg Email SERVICE: Started.";
    m_state = STARTED;
}

WickrIOMsgEmailService::~WickrIOMsgEmailService()
{
    // Stop threads
    stopThreads();

    qDebug() << "WickrIO Msg Email SERVICE: Shutdown.";
    m_state = SHUTDOWN;
}

/**
 * @brief WickrIOMsgEmailService::startThreads
 * Starts all thread for WickrIO Message to email service.
 */
void WickrIOMsgEmailService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_msgEmailThread = new WickrIOMsgEmailThread(&m_threadQT,this);

    // Connect internal threads signals and slots

    // Perform startup here, creating and configuring ressources.
    m_threadQT.start();
}

/**
 * @brief WickrIOMsgEmailService::stopThreads
 * Stops all threads on WickrIO Message to email service.
 */
void WickrIOMsgEmailService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Shredder Service
    m_threadQT.quit();
    qDebug("SHREDDER THREAD: Shutdown Thread (%p)", &m_threadQT);
}

/**
 * @brief WickrIOMsgEmailService::asStringState
 * @param state
 * @return
 */
QString WickrIOMsgEmailService::asStringState(ServiceState state)
{
    switch(state) {
    case UNINITIALIZED: return tr("Uninitialized");
    case STARTED:       return tr("Started");
    case SHUTDOWN:      return tr("Shutdown");
    default:            return tr("Unknown");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

QString WickrIOMsgEmailThread::asStringState(ThreadState state)
{
    switch(state) {
    case UNINITIALIZED: return tr("Uninitialized");
    case STARTED:       return tr("Started");
    case STOPPED:       return tr("Stopped");
    default:            return tr("Unknown");
    }
}

/**
 * @brief WickrIOMsgEmailThread::WickrIOMsgEmailThread
 * Constructor
 */
WickrIOMsgEmailThread::WickrIOMsgEmailThread(QThread *thread, WickrIOMsgEmailService *msgEmailSvc)
    : m_lock(QReadWriteLock::Recursive)
    , m_parent(msgEmailSvc)
    , m_state(UNINITIALIZED)
    , m_status(SENDING_STOPPED)
{
    qRegisterMetaType<WickrIOMsgEmailThread::ThreadState>("ThreadState");
    thread->setObjectName("WickrIOMsgEmailThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(&m_thread, &QThread::finished, this, &QObject::deleteLater);

    // Send message to email signals
    connect(this, &WickrIOMsgEmailThread::signalSendMessages, this, &WickrIOMsgEmailThread::slotSendMessages);

    m_state = STARTED;
}

/**
 * @brief WickrIOMsgEmailThread::~WickrIOMsgEmailThread
 * Destructor
 */
WickrIOMsgEmailThread::~WickrIOMsgEmailThread() {
    qDebug() << "WickrIO Msg to Email THREAD: Worker Destroyed.";
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void
WickrIOMsgEmailThread::sendMessages(WickrIOMsgEmailMessages *messages)
{
    emit signalSendMessages(messages);
}

void
WickrIOMsgEmailThread::slotSendMessages(WickrIOMsgEmailMessages *messages)
{
    m_messagesList.append(messages);

    if (m_status == SENDING_STOPPED) {
        processMessageList();
    }
}


#if 0
MimeFile *
WickrIOMsgEmailThread::getAttachmentFile(const QByteArray &data, QString extension)
{
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("yyyyMMddhhmmsszzz");
    // Save the attachment to the temp dir
    QString tempPath(QDir::tempPath());

    QString attachmentFileName(tempPath +
#ifdef Q_OS_LINUX
            "/" +
#endif
     "attachment_" + dateTimeString + "." + extension);

    QFile tempFile(attachmentFileName);
    tempFile.open(QIODevice::WriteOnly);
    tempFile.write(data);
    tempFile.close();

    MimeFile *attachfile  = new MimeFile(new QFile(attachmentFileName));
    m_curMsg->attachmentFiles.append(attachfile);
    m_curMsg->attachmentTempFileNames.append(attachmentFileName);
    return attachfile;
}
#endif

/**
 * @brief WickrIOMsgEmailService::processMessageList
 * This function will attempt to send the currently first message on the list of messages
 * on the m_messages list.  If there are no messages left a false value will be reurned.
 * @return
 */
bool
WickrIOMsgEmailThread::processMessageList()
{
    m_status = SENDING_RUNNING;
    if (!m_curMessages || m_curMessages->m_messages.length() == 0) {
        if (! m_curMessages) {
            delete m_curMessages;
        }

        if (m_messagesList.length() == 0) {
            m_status = SENDING_STOPPED;
            return false;
        }

        // Pop a list off of the messages list
        m_curMessages = m_messagesList.first();
        m_messagesList.removeFirst();

        SmtpClient::ConnectionType type;
        if (m_curMessages->m_emailSettings.type.toLower() == "smtp")
            type = SmtpClient::TcpConnection;
        else if (m_curMessages->m_emailSettings.type.toLower() == "ssl")
            type = SmtpClient::SslConnection;
        else if (m_curMessages->m_emailSettings.type.toLower() == "tls")
            type = SmtpClient::TlsConnection;

        m_smtp = new SmtpClient(m_curMessages->m_emailSettings.server, m_curMessages->m_emailSettings.port, type);
        if (m_smtp == NULL) {
            delete m_curMessages;
        } else {
            m_smtp->setUser(m_curMessages->m_emailSettings.account);
            m_smtp->setPassword(m_curMessages->m_emailSettings.password);
            m_smtp->connectToHost();
            m_smtp->login();

            while (processMessageFromList()) {
            }
            cleanup();
            delete m_curMessages;
        }
    }
    return true;
}

bool
WickrIOMsgEmailThread::processMessageFromList()
{
    if (m_curMessages->m_messages.length() > 0) {
        m_curMsg = m_curMessages->m_messages.first();
        m_curMessages->m_messages.removeFirst();

        MimeMessage message;
        message.setSender(new EmailAddress(m_curMessages->m_emailSettings.sender));
        message.addRecipient(new EmailAddress(m_curMessages->m_emailSettings.recipient));
        message.setSubject(m_curMessages->m_emailSettings.subject);

        MimeText text;
        text.setText(m_curMsg->m_data);
        message.addPart(&text);

#if 0
        if (m_curMsg->msg->hasAttachments()) {
            QList<WickrCore::WickrAttachment> attachments = m_curMsg->msg->getAttachments();
            for (WickrCore::WickrAttachment attachment : attachments) {
                if (attachment.getType() == WICKR_ATTACH_PIC) {
                    qDebug() << "Attachment is a picture";
                    getAttachmentFile(attachment.getData(), "jpg");
                } else if (attachment.getType() == WICKR_ATTACH_VID) {
                    qDebug() << "Attachment is a Video";
                    getAttachmentFile(attachment.getData(), "mp4");
                } else if (attachment.getType() == WICKR_ATTACH_PDF) {
                    qDebug() << "Attachment is a PDF";
                    getAttachmentFile(attachment.getData(), "pdf");
                } else if (attachment.getType() == WICKR_ATTACH_PAYLOAD) {
                    qDebug() << "Attachment is a Payload";
                    QJsonDocument jdoc = Qbson::bsonToJsonDoc(attachment.getData());

                    // Convert from json to qvariantmap
                    QJsonObject jobj = jdoc.object();
                    int pLoadKey = jobj["ploadkey"].toInt();
                    int subType = jobj["subtype"].toInt();
                    QString pLoad = jobj["pload"].toString();
                    QString fileType = jobj["filetype"].toString();

                    // Generic file type
                    if (pLoadKey == 0 && subType== 8) {
                        getAttachmentFile(pLoad.toUtf8(), fileType);
                    }
                }
            }
        } else if (m_curMsg->msg->getMsgClass() == WICKR_MSG_TYPE_AUDIO) {
            getAttachmentFile(m_curMsg->msg->getAudioBody(), "wav");
        }
#endif

        for (MimeFile* file : m_curMsg->m_attachmentFiles) {
            message.addPart(file);
        }

        m_smtp->sendMail(message);

        // Free the current message memory and delete the message
        m_curMsg->m_msg->dodelete();
//        delete m_curMsg->msg;
        for (MimeFile *file : m_curMsg->m_attachmentFiles) {
            file->deleteLater();
        }
        for (QString filename : m_curMsg->m_attachmentTempFileNames) {
            QFile tempFile(filename);
            tempFile.remove();
        }
        delete m_curMsg;
        m_curMsg = NULL;

        return true;
     }
     return false;
}

/**
 * @brief WickrIOMsgEmailService::cleanup
 */
void
WickrIOMsgEmailThread::cleanup()
{
    if (m_smtp != NULL) {
        m_smtp->quit();
        delete m_smtp;
        m_smtp = NULL;
    }
    if (m_curMsg != NULL) {
#if 0
        for (MimeFile *file : m_curMsg->attachmentFiles) {
            delete file;
        }
#endif
        delete m_curMsg;
        m_curMsg = NULL;
    }
    while (m_curMessages->m_messages.length() > 0) {
        m_curMsg = m_curMessages->m_messages.first();
        delete m_curMsg;
        m_curMessages->m_messages.removeFirst();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOMsgEmailMessages::WickrIOMsgEmailMessages(const WickrIOEmailSettings& emailSettings) :
    m_emailSettings(emailSettings)
{
}

WickrIOMsgEmailMessages::~WickrIOMsgEmailMessages()
{
    // TODO: Clean up and messages that are remaining on the list
}

void
WickrIOMsgEmailMessages::addMessage(WickrIOMsgEmailMessage *message)
{
    m_messages.append(message);
}

