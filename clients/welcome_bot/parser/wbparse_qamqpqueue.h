#ifndef WBPARSE_QAMQPQUEUE_H
#define WBPARSE_QAMQPQUEUE_H

#include <QObject>
#include <QScopedPointer>
#include <QList>

#include "qamqpclient.h"
#include "qamqpqueue.h"
#include "qamqpexchange.h"
#include "parseroperationdata.h"

typedef enum { QSTATE_IDLE=0, QSTATE_CONNECTING, QSTATE_FAILED_CONNECT, QSTATE_RUNNING, } QueueState;

class WBParse_QAMQPQueue : public QObject
{
    Q_OBJECT

public:
    WBParse_QAMQPQueue(ParserOperationData *operation);
    ~WBParse_QAMQPQueue();

    void cleanup();
    bool timerCall();

    bool isRunning() { return m_queueState == QSTATE_RUNNING; }

private:
    QAmqpClient m_client;
    QAmqpQueue *m_queue;
    QAmqpExchange *m_exchange;
    QList<QAmqpMessage> m_ackMessages;
    QAmqpMessage m_currentMessage;
    ParserOperationData *m_operation;
    QueueState m_queueState;

    QString m_queueExchangeName;
    QString m_queueName;

private slots:
    void connected();
    void disconnected();
    void exchangeDeclared();
    void queueDeclared();
    void messageReceived();
    void ackMessage();
    void error(QAMQP::Error error);
    void socketError(QAbstractSocket::SocketError error);

};

#endif // WBPARSE_QAMQPQUEUE_H
