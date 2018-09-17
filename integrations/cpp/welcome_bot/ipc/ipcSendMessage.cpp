#include <QDebug>
#include <QDir>

#include "ipcSendMessage.h"

IpcSendMessage::IpcSendMessage(QObject *parent) : QObject(parent)
{
    m_zctx = nzmqt::createDefaultContext();

    m_zsocket = m_zctx->createSocket(nzmqt::ZMQSocket::TYP_REQ, this);
    m_zsocket->setObjectName("Requester.Socket.socket(REQ)");
    connect(m_zsocket, &nzmqt::ZMQSocket::messageReceived,
            this, &IpcSendMessage::slotAsyncResponseReceived, Qt::QueuedConnection);

    m_zctx->start();


    QString queueDirName = QString("%1/tmp").arg(QDir::currentPath());
    QDir    queueDir(queueDirName);
    if (!queueDir.exists()) {
        if (!queueDir.mkpath(queueDirName)) {
            qDebug() << "Cannot create message queue directory!";
        }
    }

    // Create the socket file for the addon receive queue
    {
        QString queueName = QString("ipc://%1/tmp/0").arg(QDir::currentPath());
        m_zsocket->bindTo(queueName);

        // Set the permission of the queue file so that normal user programs can access
        QString queueFileName = QString("%1/tmp/0").arg(QDir::currentPath());
        QFile zmqFile(queueFileName);
        if(!zmqFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                   QFile::ReadUser | QFile::WriteUser | QFile::ExeUser  |
                                   QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
                                   QFile::ReadOther | QFile::WriteOther | QFile::ExeOther)) {
            qDebug("Something wrong setting permissions of the queue file!");
        }
    }
}
;
void
IpcSendMessage::sendMessage(QString msg)
{
    QList<QByteArray> request;
    request += msg.toLocal8Bit();
    if (msg.length() > 0)
        qDebug() << "Sending async message:" << msg;
    if (!m_zsocket->sendMessage(request)) {
        qDebug() << "Failed to send async message!";
        exit(1);
    } else {
        time(&m_sentMessageTime);   // Set the time that message is sent for timeout
        m_messageSent = true;
    }
}


void
IpcSendMessage::slotAsyncResponseReceived(const QList<QByteArray>& messages)
{
    for (QByteArray mesg : messages) {
        exit(0);
    }
}
