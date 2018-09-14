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

    QString m_action;
    QStringList m_userIDs;
    QStringList m_userNames;
    QString m_vgroupid;
    QString m_message;
    QDateTime m_runTime;
    int m_ttl;
    int m_bor;
    bool m_has_bor;

private slots:
    void connected();
    void disconnected();
    void exchangeDeclared();
    void queueDeclared();
    void messageReceived();
    void ackMessage();
    void error(QAMQP::Error error);
    void socketError(QAbstractSocket::SocketError error);

    bool parseMessage(QByteArray& message);
    bool processSendMessageJsonDocV3(const QJsonObject &operationObject);
    bool processSendMessageJsonDoc(const QJsonObject &operationObject);
    int     processSendMessage();

};

#endif // WBPARSE_QAMQPQUEUE_H
