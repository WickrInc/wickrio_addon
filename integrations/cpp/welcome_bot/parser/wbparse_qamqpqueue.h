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
    typedef enum {
        ParserAction_SendMessage,
        ParserAction_SendWelcomeMessage,
        ParserAction_NewDevice,
        ParserAction_ForgotPassword
    } ParserIOActions;

    QAmqpClient m_client;
    QAmqpQueue *m_queue;
    QAmqpExchange *m_exchange;
    QList<QAmqpMessage> m_ackMessages;
    QAmqpMessage m_currentMessage;
    ParserOperationData *m_operation;
    QueueState m_queueState;

    QString m_queueExchangeName;
    QString m_queueName;

    ParserIOActions m_action;

    QString     m_uname;
    bool        m_isAdmin;
    QString     m_message;
    QDateTime   m_runTime;

    int m_ttl;
    int m_bor;
    bool m_has_bor;

    bool parseMessage(QByteArray& message);

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

#define PARSERJSON_COMMAND              "command"
#define PARSERJSON_UNAME                "uname"
#define PARSERJSON_MESSAGE              "message"
#define PARSERJSON_CLIENTTYPE           "clientType"
#define PARSERJSON_RUNTIME              "runtime"
#define PARSERJSON_TTL                  "ttl"
#define PARSERJSON_BOR                  "bor"

#define PARSERJSON_CMD_PUSH_MESSAGE     "push_bot_message"      // Legacy
#define PARSERJSON_CMD_WELCOME_MESSAGE  "sendWelcomeMessage"
#define PARSERJSON_CMD_NEW_DEVICE       "newDevice"
#define PARSERJSON_CMD_FORGOT_PASSWORD  "forgotPassword"

#endif // WBPARSE_QAMQPQUEUE_H
