#include "wickrbotipc.h"

WickrBotIPC::WickrBotIPC(QObject *parent) :
    QObject(parent),
    m_server(NULL),
    m_socket(NULL),
    m_sendMessage("")
{
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

bool WickrBotIPC::sendMessage(const QString& dest, const QString &message)
{
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

    m_socket->write(data);
    m_socket->flush();

    m_socket->deleteLater();

    emit signalSentMessage();
}


QByteArray WickrBotIPC::getResponse(const QString& source, bool *result)
{
    QByteArray bytes;
    return bytes;
}


