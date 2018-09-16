#include "wbparse_qamqpqueue.h"
#include "wickrbotjsondata.h"
#include "qamqpexchange.h"

#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "welcomeClientConfigInfo.h"
#include "wickrioapi.h"
#include "bot_iface.h"

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
        qDebug() << "The Queue State Connection has failed!";
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
    qDebug() << QString("ERROR received from QAMQP %1").arg(error);
}

void WBParse_QAMQPQueue::socketError(QAbstractSocket::SocketError error)
{
    qDebug() << QString("ERROR received from QAMQP %1").arg(error);
    m_queueState = QSTATE_FAILED_CONNECT;
}

void WBParse_QAMQPQueue::connected()
{
    qDebug() << "WBParse_QAMQPQueue: connected() called";
    m_queueState = QSTATE_RUNNING;

    m_exchange = m_client.createExchange(m_queueExchangeName);

    connect(m_exchange, SIGNAL(declared()), this, SLOT(exchangeDeclared()));
    const QAmqpTable args;
    m_exchange->declare(QAmqpExchange::Direct, QAmqpExchange::Durable, args);
}

void WBParse_QAMQPQueue::exchangeDeclared()
{
    qDebug() << "WBParse_QAMQPQueue: exchangeDeclared() called";

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
    qDebug() << "WBParse_QAMQPQueue: disconnected() called";
}

void WBParse_QAMQPQueue::queueDeclared()
{
    qDebug() << "WBParse_QAMQPQueue: queueDeclared() called";

    // m_queue->setPrefetchCount(1);
    m_queue->consume();
    connect(m_queue, SIGNAL(messageReceived()), this, SLOT(messageReceived()));
}

bool WBParse_QAMQPQueue::parseMessage(QByteArray& message)
{
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(message, &jsonError);

    if (jsonError.error != jsonError.NoError) {
        qDebug() << "Error with JSON document" << jsonError.errorString();
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();

    // Start the operation
    if (!jsonObject.contains(PARSERJSON_COMMAND)) {
        qDebug() << "Request does not contain Command!";
        return false;
    }

    QJsonValue value = jsonObject[PARSERJSON_COMMAND];
    QString operation = value.toString("");
    if (operation == PARSERJSON_CMD_PUSH_MESSAGE) {
        m_action = ParserIOActions::ParserAction_SendMessage;
    } else if (operation == PARSERJSON_CMD_WELCOME_MESSAGE) {
        m_action = ParserIOActions::ParserAction_SendWelcomeMessage;
    } else if (operation == PARSERJSON_CMD_NEW_DEVICE) {
        m_action = ParserIOActions::ParserAction_NewDevice;
    } else if (operation == PARSERJSON_CMD_FORGOT_PASSWORD) {
        m_action = ParserIOActions::ParserAction_ForgotPassword;
    } else {
        qDebug().noquote() << QString("Request contains invalid %1 value (%2)").arg(PARSERJSON_COMMAND).arg(operation);
        return false;
    }

    // Get the Uname of the Wickr client to send to
    if (jsonObject.contains(PARSERJSON_UNAME)) {
        value = jsonObject[PARSERJSON_UNAME];
        m_uname=value.toString();
        if (m_operation->debug)
            qDebug() << "Uname:" << m_uname;
    }

    // Get the type of client it is for, if it is set
    if (jsonObject.contains(PARSERJSON_CLIENTTYPE)) {
        value = jsonObject[PARSERJSON_CLIENTTYPE];
        m_isAdmin = (value.toString("") == "admin");
    } else {
        m_isAdmin = false;
    }

    // Parse out any message
    if (jsonObject.contains(PARSERJSON_MESSAGE)) {
        value = jsonObject[PARSERJSON_MESSAGE];
        m_message = value.toString();
        if (m_operation->debug)
            qDebug() << "Message:" << m_message;
    } else {
        if (m_action == ParserIOActions::ParserAction_SendMessage) {
            qDebug() << "Send message command with no message to send!";
            return false;
        }
        if (m_action == ParserIOActions::ParserAction_SendWelcomeMessage) {
            if (m_isAdmin)
                m_message = m_operation->m_welcomeAdminMessage;
            else
                m_message = m_operation->m_welcomeUserMessage;
        } else if (m_action == ParserIOActions::ParserAction_NewDevice) {
            m_message = m_operation->m_newDeviceMessage;
        } else if (m_action == ParserIOActions::ParserAction_ForgotPassword) {
            m_message = m_operation->m_forgotPwMessage;
        }
    }
    if (m_message.isEmpty()) {
        qDebug() << "No message available to send!";
        return false;
    }

    if (jsonObject.contains(PARSERJSON_RUNTIME)) {
        value = jsonObject[PARSERJSON_RUNTIME];

        // Use the current date and time if the one is invalid
        m_runTime = value.toVariant().toDateTime();
    } else {
        QDateTime dt = QDateTime::currentDateTime();
        dt = dt.addSecs(20);

        if (m_operation->debug)
            qDebug() << "New Message with time=" << dt.toString(DB_DATETIME_FORMAT);
        m_runTime = dt;
    }

    // Parse out any TTL
    if (jsonObject.contains(PARSERJSON_TTL)) {
        value = jsonObject[PARSERJSON_TTL];
        m_ttl = value.toInt(0);
        if (m_operation->debug)
            qDebug() << "TTL:" << m_ttl;
    }

    // Parse out any BOR
    if (jsonObject.contains(PARSERJSON_BOR)) {
        value = jsonObject[PARSERJSON_BOR];
        m_bor = value.toInt(0);
        m_has_bor = true;
        if (m_operation->debug)
            qDebug() << "BOR:" << m_bor;
    } else {
        m_has_bor = false;
    }

    return true;
}

void WBParse_QAMQPQueue::messageReceived()
{
    m_currentMessage = m_queue->dequeue();
    QByteArray queueMessage = m_currentMessage.payload();

    qDebug() << " [x] Received " << queueMessage;

    if (parseMessage(queueMessage)) {
        // Send the message via the API
        std::string command;
        std::string ttl="", bor="";

        if (m_operation->m_botIface->cmdStringSendMessageToUname(command, m_uname.toStdString(), m_message.toStdString(), ttl, bor) != BotIface::SUCCESS) {
            qDebug() << "Failed to create Send Message command!";
            //TODO: what to do if failed to send message!  Is there a NACK?
        } else {
            string response;
            if (m_operation->m_botIface->send(command, response) != BotIface::SUCCESS) {
    //            qDebug() << "Send failed: " << m_operation->m_botIface->getLastErrorString().;
                qDebug() << "Message send failed!";
            } else {
                qDebug() << "Message sent successfully";
            }
        }
    }
    m_ackMessages.append(m_currentMessage);
}

void WBParse_QAMQPQueue::ackMessage()
{
    qDebug() <<  QString("Acknowledging %1 messages").arg(m_ackMessages.count());

    for (int i=0; i<m_ackMessages.size(); i++) {
        QAmqpMessage msg = m_ackMessages.at(i);
        m_queue->ack(msg);
    }
    m_ackMessages.clear();
}

