#include "wbparse_qamqpqueue.h"
#include "wickrbotjsondata.h"
#include "qamqpexchange.h"

#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "welcomeClientConfigInfo.h"
#include "wickrioapi.h"

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

int WBParse_QAMQPQueue::processSendMessage() {
    int messageCount = 0;

    if (m_userIDs.size() == 0 && m_userNames.size() == 0 && m_vgroupid.isEmpty())
        return 0;

    for (int i=0; i<m_userIDs.size(); i++) {
        QString user = m_userIDs.at(i);
        if (m_message.size() > 0) {

            // Send the message!
            messageCount++;
        } else {

        }
    }

    for (int i=0; i<m_userNames.size(); i++) {
        QString user = m_userNames.at(i);
        if (m_message.size() > 0) {

            // TODO: FOr now create list of users, size of one
            QStringList users;
            users.append(user);

            // Send the message
            messageCount++;
        } else {

        }
    }

    if (! m_vgroupid.isEmpty()) {
        if (m_message.size() > 0) {
            // Send the message

            // put the command into the database
            messageCount++;
        }
    }

    return messageCount;
}

bool WBParse_QAMQPQueue::processSendMessageJsonDocV3(const QJsonObject &operationObject)
{
    QJsonValue value;

    // Parse the contacts
    if (operationObject.contains("uname")) {
        value = operationObject["uname"];
        m_userIDs.clear();
        QString uname=value.toString();
        m_userIDs.append(uname);
        if (m_operation->debug)
            qDebug() << "Uname:" << uname;
    }

    // Parse out any message
    if (operationObject.contains("message")) {
        value = operationObject["message"];
        m_message = value.toString();
        if (m_operation->debug)
            qDebug() << "Message:" << m_message;
    }

    if (operationObject.contains("runtime")) {
        value = operationObject["runtime"];

        // Use the current date and time if the one is invalid
        m_runTime = value.toVariant().toDateTime();
    } else {
        QDateTime dt = QDateTime::currentDateTime();
        dt = dt.addSecs(20);

        qDebug() << "New Message with time=" << dt.toString(DB_DATETIME_FORMAT);
        m_runTime = dt;
    }

    // Parse out any attachments that may be part of this message
    return true;
}

bool WBParse_QAMQPQueue::processSendMessageJsonDoc(const QJsonObject &operationObject)
{
    QJsonArray entryArray;
    QJsonValue value;

    // Parse the contacts
    if (operationObject.contains(APIJSON_MSGUSERS)) {
        m_vgroupid = QString("");
        value = operationObject[APIJSON_MSGUSERS];
        entryArray = value.toArray();
        for (int i=0; i< entryArray.size(); i++) {
            QJsonValue arrayValue;

            arrayValue = entryArray[i];

            if (arrayValue.isObject()) {
                // Get the title for this contact entry
                QJsonObject arrayObject = arrayValue.toObject();

                if (arrayObject.contains(APIJSON_MSGID)) {
                    QJsonValue idobj = arrayObject[APIJSON_MSGID];
                    QString id = idobj.toString();
                    m_userIDs.append(id);
                    if (m_operation->debug)
                        qDebug() << "User" << i+1 << id;
                } else if (arrayObject.contains(APIJSON_NAME)) {
                    QJsonValue nameobj = arrayObject[APIJSON_NAME];
                    QString name = nameobj.toString();
                    if (name.length() == 0) {
                        qDebug() << "Received 0 length user!";
                        return false;
                    }
                    m_userNames.append(name);
                    if (m_operation->debug)
                        qDebug() << "User" << i+1 << name;
                }
            }
        }
    } else if (operationObject.contains(APIJSON_VGROUPID)) {
        value = operationObject[APIJSON_VGROUPID];
        m_vgroupid = value.toString();
        if (m_vgroupid.length() == 0) {
            qDebug() << "Received 0 length VGroupID!";
            return false;
        }
        m_userNames.clear();
        m_userIDs.clear();
    } else {
        qDebug() << "Does not contain users or vgroupid!";
        return false;
    }

    if (operationObject.contains(APIJSON_STATUS_RUNTIME)) {
        value = operationObject[APIJSON_STATUS_RUNTIME];

        // Use the current date and time if the one is invalid
        m_runTime = value.toVariant().toDateTime();
    } else {
        QDateTime dt = QDateTime::currentDateTime();
        m_runTime = dt;
    }

    // Parse out any message
    if (operationObject.contains(APIJSON_MESSAGE)) {
        value = operationObject[APIJSON_MESSAGE];
        m_message = value.toString();
        if (m_message.length() == 0) {
            qDebug() << "Received 0 length message!";
            return false;
        }
        if (m_operation->debug)
            qDebug() << "Message:" << m_message;
    }

    // Parse out any TTL
    if (operationObject.contains("ttl")) {
        value = operationObject["ttl"];
        m_ttl = value.toInt(0);
        if (m_operation->debug)
            qDebug() << "TTL:" << m_ttl;
    }

    // Parse out any BOR
    if (operationObject.contains("bor")) {
        value = operationObject["bor"];
        m_bor = value.toInt(0);
        m_has_bor = true;
        if (m_operation->debug)
            qDebug() << "BOR:" << m_bor;
    } else {
        m_has_bor = false;
    }

    return true;
}


bool WBParse_QAMQPQueue::parseMessage(QByteArray& message)
{
    QString m_action;
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(message, &jsonError);

    if (jsonError.error != jsonError.NoError) {
        qDebug() << "Error with JSON document" << jsonError.errorString();
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    QJsonObject operationObject;

    // Start the operation
    if (jsonObject.contains("operation")) {
        value = jsonObject["operation"];
        operationObject = value.toObject();
    } else if (jsonObject.contains("command")) {
        value = jsonObject["command"];
        QString operation = value.toString("invalid");
        if (operation == "push_bot_message") {
            m_action = "sendmessage";
            return processSendMessageJsonDocV3(jsonObject);
        }
        return false;
    } else {
        qDebug() << "processContactReply: does not contain operation!";
        return false;
    }

    // Get the Action
    if (operationObject.contains("action")) {
        value = operationObject["action"];
        m_action = value.toString("invalid");
        if (m_action == "invalid") {
            qDebug() << QString("action is invalid, value is %1").arg(m_action);
            return false;
        }
    }
    // Assume it is a send message
    else {
        m_action = "sendmessage";

    }

    if (m_action == "sendmessage") {
        return processSendMessageJsonDoc(operationObject);
    }
    return false;

}

void WBParse_QAMQPQueue::messageReceived()
{
    m_currentMessage = m_queue->dequeue();
    QByteArray message = m_currentMessage.payload();

    qDebug() << " [x] Received " << message;

    parseMessage(message);

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

