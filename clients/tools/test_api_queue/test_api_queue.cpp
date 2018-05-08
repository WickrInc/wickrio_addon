#include <QCoreApplication>
#include <QEventLoop>
#include <QUuid>

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

#include "test_api_queue.h"

TestAPIQueue::TestAPIQueue(QObject *parent)
    : QObject(parent),
      m_client(0),
      m_responseQueue(0),
      m_defaultExchange(0)
{
    m_client = new QAmqpClient(this);
    m_client->setHost("localhost");
    connect(m_client, SIGNAL(connected()), this, SLOT(clientConnected()));
}

TestAPIQueue::~TestAPIQueue()
{
}

bool TestAPIQueue::connectToServer()
{
    QEventLoop loop;
    connect(this, SIGNAL(connected()), &loop, SLOT(quit()));
    m_client->connectToHost();
    loop.exec();

    return m_client->isConnected();
}

void TestAPIQueue::call(const QString& request)
{
    qDebug().nospace().noquote() << "Request: " << request;
    m_correlationId = QUuid::createUuid().toString();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::ReplyTo, m_responseQueue->name());
    properties.insert(QAmqpMessage::CorrelationId, m_correlationId);

    m_defaultExchange->publish(request, "rpc_queue", properties);
}

void TestAPIQueue::clientConnected()
{
    m_responseQueue = m_client->createQueue();
    connect(m_responseQueue, SIGNAL(declared()), this, SLOT(queueDeclared()));
    connect(m_responseQueue, SIGNAL(messageReceived()), this, SLOT(responseReceived()));
    m_responseQueue->declare(QAmqpQueue::Exclusive | QAmqpQueue::AutoDelete);
    m_defaultExchange = m_client->createExchange();
}

void TestAPIQueue::queueDeclared()
{
    m_responseQueue->consume();
    Q_EMIT connected();
}

void TestAPIQueue::responseReceived()
{
    QAmqpMessage message = m_responseQueue->dequeue();
    if (message.property(QAmqpMessage::CorrelationId).toString() != m_correlationId) {
        // requeue message, it wasn't meant for us
//        m_responseQueue->reject(message, true);
        return;
    }

    m_lastReponse = message.payload();
    emit signalGotResponse();
}
