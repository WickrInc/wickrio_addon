#include "wickrIOMsgCallbackService.h"

#include "session/wickrSession.h"
#include "session/wickrDeviceMan.h"
#include "common/wickrRuntime.h"
#include "session/environmentmgr.h"

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOMsgCallbackService::WickrIOMsgCallbackService
 * Constructor
 */
WickrIOMsgCallbackService::WickrIOMsgCallbackService()
    : m_lock(QReadWriteLock::Recursive)
    , m_state(UNINITIALIZED)
    , m_cbThread(nullptr)
{
    // Start threads
    startThreads();

    setObjectName("WickrShredderService");
    qDebug() << "SHREDDER SERVICE: Started.";
    m_state = STARTED;
}

/**
 * @brief WickrIOMsgCallbackService::~WickrIOMsgCallbackService
 * Destructor
 */
WickrIOMsgCallbackService::~WickrIOMsgCallbackService() {
    // Stop threads
    stopThreads();

    qDebug() << "WickrIO Msg Callback SERVICE: Shutdown.";
    m_state = SHUTDOWN;
}

/**
 * @brief WickrIOMsgCallbackService::startThreads
 * Starts all thread for WickrIO msg callback service.
 */
void WickrIOMsgCallbackService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new WickrIOMsgCallbackThread(&m_threadST,this);

    // Connect internal threads signals and slots

    // Perform startup here, creating and configuring ressources.
    m_threadST.start();
}

/**
 * @brief WickrIOMsgCallbackService::stopThreads
 * Stops all threads on WickrIO msg callabck service.
 */
void WickrIOMsgCallbackService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // WickrIO Msg Callback Service
    m_threadST.quit();
    qDebug("WickrIO Msg Callback THREAD: Shutdown Thread (%p)", &m_threadST);
}

/**
 * @brief WickrIOMsgCallbackService::asStringState
 * @param state
 * @return
 */
QString WickrIOMsgCallbackService::asStringState(ServiceState state)
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

/**
 * @brief WickrShredderThread::asStringState
 * @param state
 * @return
 */
QString WickrIOMsgCallbackThread::asStringState(ThreadState state)
{
    switch(state) {
    case UNINITIALIZED: return tr("Uninitialized");
    case STARTED:       return tr("Started");
    case STOPPED:       return tr("Stopped");
    default:            return tr("Unknown");
    }
}

WickrIOMsgCallbackThread::WickrIOMsgCallbackThread(QThread *thread,
                                                   WickrIOMsgCallbackService *msgCallbackService) :
    m_state(UNINITIALIZED),
    m_parent(msgCallbackService),
    m_status(MSGCALLBACK_STOPPED),
    m_mgr(nullptr),
    m_reply(nullptr)
{
    qRegisterMetaType<WickrIOMsgCallbackThread::ThreadState>("ThreadState");
    thread->setObjectName("WickrIOMsgCallbackThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(&m_thread, &QThread::finished, this, &QObject::deleteLater);

    // Signal to send messages
    connect(this, &WickrIOMsgCallbackThread::signalSendMessages, this, &WickrIOMsgCallbackThread::slotSendMessages);

    m_mgr = new QNetworkAccessManager(this);
    connect(m_mgr, &QNetworkAccessManager::sslErrors, this, &WickrIOMsgCallbackThread::slotSslErrors);
    QObject::connect(m_mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(gotReply(QNetworkReply*)));

    m_state = STARTED;
}

WickrIOMsgCallbackThread::~WickrIOMsgCallbackThread()
{
    //TODO: What to do to the list of messages at this point!
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void WickrIOMsgCallbackThread::sendMessages(QList<WickrIOMsgCallbackMessage *>messages)
{
    emit signalSendMessages(messages);
}

/**
 * @brief WickrIOMsgCallbackService::serviceExecute
 * Initiate the sending of the messages to the identified callback location.
 * @param svc
 */
void
WickrIOMsgCallbackThread::slotSendMessages(QList<WickrIOMsgCallbackMessage *>messages)
{
    for (WickrIOMsgCallbackMessage *msg : messages) {
        m_messages.append(msg);
    }

    // if the message sending is not active then kick it off
    if (m_status == MSGCALLBACK_STOPPED) {
        processMessagesList();
    }
}

bool
WickrIOMsgCallbackThread::processMessagesList()
{
    m_status = MSGCALLBACK_RUNNING;

    if (m_messages.length() > 0) {
        m_curMsg = m_messages.first();
        m_messages.removeFirst();

#if 1
        QByteArray postDataSize = QByteArray::number(m_curMsg->m_data.size());
        QUrl serviceURL(m_url);

        QNetworkRequest request(serviceURL);
        request.setRawHeader("User-Agent", "WickrIO v1.1");
        request.setRawHeader("X-Custom-User-Agent", "WickrIO v1.1");
        request.setRawHeader("Content-Type", "application/json");
        request.setRawHeader("Content-Length", postDataSize);

        m_reply = m_mgr->post(request, m_curMsg->m_data);

        QObject::connect(m_reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(sendProgress(qint64,qint64)));
        QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
#else
        initRequest(m_url);

        QHttpPart bodypart;
//        bodypart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
        bodypart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
        bodypart.setBody(m_curMsg->data);
        entity->append(bodypart);

        mHeaders["User-Agent"] = "WickrIO v1.1";
        mHeaders["X-Custom-User-Agent"] = "WickrIO v1.1";
//        mHeaders["Content-Type"] = "application/json";
//        mHeaders["Content-Length"] = postDataSize;

        runRequest();
#endif
        return true;
    }
    m_status = MSGCALLBACK_STOPPED;
    return false;
}

void
WickrIOMsgCallbackThread::finished()
{
    qDebug() << "in WickrIOMsgCallbackService:: finished()";
    // Free the current message memory and delete the message
    if (m_curMsg != NULL) {
        m_curMsg->m_msg->dodelete();
        delete m_curMsg;
        m_curMsg = NULL;
    }

    // If there are no more messages to send then we are done
    if (m_status == MSGCALLBACK_STOPPED || ! processMessagesList()) {
        cleanup();
    }
}

void
WickrIOMsgCallbackThread::sendProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if( bytesSent == bytesTotal ) {
    } else {
    }
}

/**
 * @brief WickrIOMsgCallbackService::cleanup
 */
void
WickrIOMsgCallbackThread::cleanup()
{
    if (m_curMsg != NULL) {
        delete m_curMsg;
        m_curMsg = NULL;
    }
    while (m_messages.length() > 0) {
        m_curMsg = m_messages.first();
        delete m_curMsg;
        m_messages.removeFirst();
    }
}

/**
 * @brief WickrIOMsgCallbackService::msgSendCallbackResponse
 * This slot is called after a callback message was sent successfully.  The associated message
 * will be freed, and then the next message on the list of messages will be sent, if there are
 * any left to send.
 */
void
WickrIOMsgCallbackThread::msgSendCallbackResponse(QNetworkReply *thereply)
{
    qDebug() << "in WickrIOMsgCallbackService::msgSendCallbackResponse()";

    // Free the current message memory and delete the message
    if (m_curMsg != NULL) {
        m_curMsg->m_msg->dodelete();
        delete m_curMsg;
        m_curMsg = NULL;
    }

    // If there are no more messages to send then we are done
    if (m_status == MSGCALLBACK_STOPPED || ! processMessagesList()) {
        cleanup();
    }
}

void
WickrIOMsgCallbackThread::msgSendCallbackError(QNetworkReply *thereply)
{
    qDebug() << "in WickrIOMsgCallbackService::msgSendCallbackError()";

    cleanup();
}



void
WickrIOMsgCallbackThread::gotReply(QNetworkReply *thereply)
{
    if( !m_reply || thereply != m_reply ) {
        //qDebug() << "ignoring this one";
        return;
    }
    m_reply->deleteLater();

    // Reading attributes of the reply
    // e.g. the HTTP status code
    QVariant statusCodeV = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    // Or the target URL if it was a redirect:
    QVariant redirectionTargetUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    // see CS001432 on how to handle this

    // no error received?
    if (m_reply->error() == QNetworkReply::NoError)
    {
        // read data from QNetworkReply here
//        QByteArray bytes = m_reply->readAll();
        msgSendCallbackResponse(m_reply);
    }
    // Some http error received
    else
    {
        qDebug() << "gotReply: url=" << m_reply->url() << " error=" << m_reply->errorString();
        QString error = tr("Network Connectivity Error %1: Please check your Internet access").arg(statusCodeV.toString());
        msgSendCallbackError(m_reply);
    }
}

void
WickrIOMsgCallbackThread::slotSslErrors(QNetworkReply *reply, const QList<QSslError>& errors)
{
    unsigned count = 0;
    foreach(auto error, errors) {
        switch(error.error()) {
        case QSslError::SelfSignedCertificate:
        case QSslError::SelfSignedCertificateInChain:
            break;
        case QSslError::HostNameMismatch:
        case QSslError::InvalidPurpose:
        case QSslError::UnableToGetIssuerCertificate:
        case QSslError::CertificateUntrusted:
            if(WickrCore::WickrRuntime::getEnvironmentMgr()->pinned().count() < 1)
                ++count;
            break;
        default:
            ++count;
        }
    }

    if(count)
       qDebug() << "*** SSL ERRORS " << errors;
    else
        reply->ignoreSslErrors(errors);
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOMsgCallbackMessage::WickrIOMsgCallbackMessage(const QString& url,
                                                     const QByteArray& data,
                                                     WickrCore::WickrMessage *msg) :
    m_data(data),
    m_msg(msg)
{
    m_urlList.append(url);
}

WickrIOMsgCallbackMessage::~WickrIOMsgCallbackMessage()
{
    // TODO: Should the message be deleted?
}
