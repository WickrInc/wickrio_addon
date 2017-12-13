#include "wbparse_qamqpqueue.h"
#include "wickrbotjsondata.h"
#include "qamqpexchange.h"
#include <QHostInfo>
#include "welcomeClientConfigInfo.h"

WBParse_QAMQPQueue::WBParse_QAMQPQueue(ParserOperationData *operation)
{
    m_operation = operation;
    m_queueState = QSTATE_IDLE;

    QObject::connect(&m_client, SIGNAL(connected()), this, SLOT(connected()));
    QObject::connect(&m_client, SIGNAL(error(QAMQP::Error)), this, SLOT(error(QAMQP::Error)));
    QObject::connect(&m_client, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));

    m_client.setUsername(operation->queueUsername);
    m_client.setPassword(operation->queuePassword);
    m_queueExchangeName = operation->queueExchange;
    m_queueName = operation->queueName;
    if (!operation->queueVirtualHost.isEmpty())
        m_client.setVirtualHost(operation->queueVirtualHost);

    QHostInfo info = QHostInfo::fromName(operation->queueHost);
    QList<QHostAddress> ips = info.addresses();
    if (ips.length() == 0) {
        qDebug() << "Cannot get IP address of Message Queue";
        m_queueState = QSTATE_FAILED_CONNECT;
    } else {

        QHostAddress hostIP = QHostAddress(ips.at(0));
        m_client.connectToHost(hostIP, operation->queuePort);

        m_queueState = QSTATE_CONNECTING;
    }
}

WBParse_QAMQPQueue::~WBParse_QAMQPQueue()
{
}

bool WBParse_QAMQPQueue::timerCall()
{
    if (m_queueState == QSTATE_FAILED_CONNECT) {
        m_operation->log_handler->log("The Queue State Connection has failed!");
        return false;
    }

    if (m_queueState != QSTATE_RUNNING) {
        qDebug() << "The Queue State is not running yet";
        return true;
    }

    if (m_ackMessages.size() > 0) {
        this->ackMessage();
    }
    return true;
}

void WBParse_QAMQPQueue::error(QAMQP::Error error)
{
    m_operation->log_handler->log(QString("ERROR received from QAMQP %1").arg(error));
}

void WBParse_QAMQPQueue::socketError(QAbstractSocket::SocketError error)
{
    m_operation->log_handler->log(QString("ERROR received from QAMQP %1").arg(error));
    m_queueState = QSTATE_FAILED_CONNECT;
}

void WBParse_QAMQPQueue::connected()
{
    m_operation->log_handler->log("WBParse_QAMQPQueue: connected() called");
    m_queueState = QSTATE_RUNNING;

    m_exchange = m_client.createExchange(m_queueExchangeName);

    connect(m_exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
    const QAmqpTable args;
    m_exchange->declare(QAmqpExchange::Direct, QAmqpExchange::Durable, args);
}

void WBParse_QAMQPQueue::exchangeDeclared()
{
    m_operation->log_handler->log("WBParse_QAMQPQueue: exchangeDeclared() called");

    m_queue = m_client.createQueue(m_queueName);
    connect(m_queue, SIGNAL(declared()), this, SLOT(queueDeclared()));
    m_queue->declare(QAmqpQueue::Durable);
    qDebug() << " [*] Waiting for messages.";
}

void WBParse_QAMQPQueue::cleanup()
{
    if (m_client.isConnected()) {
        m_client.disconnectFromHost();
        QObject::connect(&m_client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    }
}

void WBParse_QAMQPQueue::disconnected()
{
    m_operation->log_handler->log("WBParse_QAMQPQueue: disconnected() called");
}

void WBParse_QAMQPQueue::queueDeclared()
{
    m_operation->log_handler->log("WBParse_QAMQPQueue: queueDeclared() called");

    // m_queue->setPrefetchCount(1);
    m_queue->consume();
    connect(m_queue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
}

void WBParse_QAMQPQueue::messageReceived()
{
    m_currentMessage = m_queue->dequeue();
    QByteArray message = m_currentMessage.payload();

    qDebug() << " [x] Received " << message;

    WickrBotJsonData *jsonHandler = new WickrBotJsonData(m_operation);
    jsonHandler->setClientType(WBIO_CLIENT_TARGET);
    jsonHandler->parse(message);

    m_ackMessages.append(m_currentMessage);
}

void WBParse_QAMQPQueue::ackMessage()
{
    m_operation->log_handler->log( QString("Acknowledging %1 messages").arg(m_ackMessages.count()));

    for (int i=0; i<m_ackMessages.size(); i++) {
        QAmqpMessage msg = m_ackMessages.at(i);
        m_queue->ack(msg);
    }
    m_ackMessages.clear();
}

