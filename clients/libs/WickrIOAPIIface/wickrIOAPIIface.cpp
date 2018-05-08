#include "wickrIOAPIIface.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QUuid>

WickrIOAPIIface::WickrIOAPIIface(QObject *parent)
    : QObject(parent)
{
    m_client = new QAmqpClient(this);
    m_client->setHost("localhost");
    connect(m_client, SIGNAL(connected()), this, SLOT(slotClientConnected()));
}

WickrIOAPIIface::~WickrIOAPIIface()
{
}

bool WickrIOAPIIface::slotConnectToServer()
{
    QEventLoop loop;
    connect(this, SIGNAL(signalConnected()), &loop, SLOT(quit()));
    m_client->connectToHost();
    loop.exec();

    return m_client->isConnected();
}

void
WickrIOAPIIface::makeRequest(const QString& request)
{
    qDebug() << " [x] Request: (" << request << ")";
    m_correlationId = QUuid::createUuid().toString();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::ReplyTo, m_responseQueue->name());
    properties.insert(QAmqpMessage::CorrelationId, m_correlationId);

    m_defaultExchange->publish(request, "rpc_queue", properties);
}

void WickrIOAPIIface::slotClientConnected()
{
    m_responseQueue = m_client->createQueue();
    connect(m_responseQueue, SIGNAL(declared()), this, SLOT(slotQueueDeclared()));
    connect(m_responseQueue, SIGNAL(messageReceived()), this, SLOT(slotResponseReceived()));
    m_responseQueue->declare(QAmqpQueue::Exclusive | QAmqpQueue::AutoDelete);
    m_defaultExchange = m_client->createExchange();
}

void WickrIOAPIIface::slotQueueDeclared()
{
    m_responseQueue->consume();
    emit signalConnected();
}

void WickrIOAPIIface::slotResponseReceived()
{
    QAmqpMessage message = m_responseQueue->dequeue();
    if (message.property(QAmqpMessage::CorrelationId).toString() != m_correlationId) {
        // requeue message, it wasn't meant for us
//        m_responseQueue->reject(message, true);
        return;
    }

    qDebug() << " [.] Got " << message.payload();
    qApp->quit();
}
