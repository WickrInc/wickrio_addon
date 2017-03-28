#include "wickrbotipc.h"

WickrBotIPC::WickrBotIPC(QObject *parent) :
    QObject(parent),
    m_server(NULL),
    m_socket(NULL),
    m_sendMessage("")
{
}

bool WickrBotIPC::check()
{
    bool retVal = true;
    if (m_server != NULL) {
        if (! m_server->isListening()) {
            qDebug() << "WickrBotIPC: IPC server is NOT listening anymore!";
            retVal = false;
        }
    }
    return retVal;
}

bool WickrBotIPC::startServer()
{
    m_server = new QTcpServer(this);
    if (! m_server->listen()) {
        qDebug() << "Cannot listen to interface";
        return false;
    }
    connect(m_server, &QTcpServer::newConnection, this, &WickrBotIPC::slotNewConnection);
    return true;
}

void WickrBotIPC::slotNewConnection()
{
    qDebug() << "Got connection";
    QTcpSocket *socket;

    // TODO: Need to handle multiple connections
    while ((socket = m_server->nextPendingConnection()) != NULL) {
        m_socket = socket;
        connect(m_socket, &QTcpSocket::readyRead, [=]() {
            int bytesAvail = m_socket->bytesAvailable();
            QString line = "";
            int cnt = 0;
            while (cnt < bytesAvail) {
                char ch;
                int bytesRead = m_socket->read(&ch, sizeof(ch));
                if (bytesRead == sizeof(ch)) {
                    cnt++;
                    line.append(ch);
                } else {
                    break;
                }
            }

            if (! line.isEmpty()) {
                emit signalGotMessage(line, (int)socket->peerPort());
            }
        });
    }
}

bool WickrBotIPC::sendMessage(int ipc_port, const QString &message)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &WickrBotIPC::slotReadyToSend);
    connect(m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
        [=](QAbstractSocket::SocketError socketError){
            qDebug() << "Received socket error:" << socketError;
            emit signalSendError();
        });

    m_socket->connectToHost("localhost", ipc_port);
    m_sendMessage = message;
    return true;
}

void WickrBotIPC::slotErrorSend(QAbstractSocket::SocketError socketError)
{
    qDebug() << "Received socket error:" << socketError;
}

/**
 * @brief WickrBotIPC::slotReadyToSend
 * Called when the QTcpSocket is ready to send the outbound message.
 * Signal is generated so caller can know the message was sent.
 */
void WickrBotIPC::slotReadyToSend()
{
    QByteArray data = m_sendMessage.toUtf8();
    qDebug() << "Ready to send:" << m_sendMessage;

    m_socket->write(data);
    m_socket->flush();

    m_socket->deleteLater();

    emit signalSentMessage();
}


QByteArray WickrBotIPC::getResponse(int ipc_port, bool *result)
{
    QByteArray bytes;

    m_socket = new QTcpSocket(this);
    m_socket->connectToHost("localhost", ipc_port);

    if (m_socket->waitForReadyRead(-1)) {
        bytes = m_socket->readAll();
        *result = true;
    } else {
        *result = false;
    }
    return bytes;
}


int WickrBotIPC::getServerPort()
{
    if (m_server != NULL) {
        return (int)m_server->serverPort();
    }
    return 0;
}
