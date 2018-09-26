/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qtLocalPeer.h"
#include <QCoreApplication>
#include <QDataStream>
#include <QTime>
#include <QDataStream>

#include <sys/types.h>
#include <time.h>
#include <unistd.h>


const char* QtLocalPeer::ack = "ack";

QtLocalPeer::QtLocalPeer(QObject* parent, const QString &appId, const QString &lockName)
    : QObject(parent), id(appId), m_lockName(lockName), m_lockFile(lockName)
{
    QString prefix = id;
    if (id.isEmpty()) {
        id = QCoreApplication::applicationFilePath();
        prefix = id.section(QLatin1Char('/'), -1);
    }
    prefix.remove(QRegExp("[^a-zA-Z]"));
    prefix.truncate(6);

    QByteArray idc = id.toUtf8();
    quint16 idNum = qChecksum(idc.constData(), idc.size());
    socketName = QLatin1String("qtsingleapp-") + prefix
                 + QLatin1Char('-') + QString::number(idNum, 16);

    socketName += QLatin1Char('-') + QString::number(::getuid(), 16);

    // Create server instance
    server = new QLocalServer(this);

    // Try to acquire lock
    m_lockFile.setStaleLockTime(0);
    if (!m_lockFile.tryLock(100)) {
        // We are not primary application instance
        m_isPrimaryInstance = false;

    } else {
        // We are primary application instance
        m_isPrimaryInstance = true;
        configureServer();
    }
}

/**
 * @brief QtLocalPeer::configureServer
 */
void QtLocalPeer::configureServer()
{
    // Log info
    qDebug().noquote().nospace() << "PRIMARY NAMED SOCKET: " << socketName;
    qDebug().noquote().nospace() << "LOCK FILE           : " << m_lockName;

    // Configure server connection
    bool res = server->listen(socketName);

#if defined(Q_OS_UNIX) && (QT_VERSION >= QT_VERSION_CHECK(4,5,0))
    // WORKAROUND - If address already in use, remove it (as we are primary...we are the ALPHA!)
    if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(QDir::cleanPath(QDir::tempPath())+QLatin1Char('/')+socketName);
        res = server->listen(socketName);
    }
#endif

    if (!res)
        qWarning("QtSingleCoreApplication: listen on local socket failed, %s", qPrintable(server->errorString()));
    QObject::connect(server, &QLocalServer::newConnection, this, &QtLocalPeer::receiveConnection);
}

bool QtLocalPeer::sendMessage(const QString &message, int timeout)
{
    // If we are primary application instance, do not send
    if (m_isPrimaryInstance)
        return false;

    QLocalSocket socket;
    bool connOk = false;
    for(int i = 0; i < 2; i++) {
        // Try twice, in case the other instance is just starting up
        socket.connectToServer(socketName);
        connOk = socket.waitForConnected(timeout/2);
        if (connOk || i)
            break;
        int ms = 250;
        struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
        nanosleep(&ts, NULL);
    }
    if (!connOk)
        return false;

    QByteArray uMsg(message.toUtf8());
    QDataStream ds(&socket);
    ds.writeBytes(uMsg.constData(), uMsg.size());
    bool res = socket.waitForBytesWritten(timeout);
    if (res) {
        res &= socket.waitForReadyRead(timeout);   // wait for ack
        if (res)
            res &= (socket.read(qstrlen(ack)) == ack);
    }
    return res;
}


void QtLocalPeer::receiveConnection()
{
    QLocalSocket* socket = server->nextPendingConnection();
    if (!socket)
        return;

    while (socket->bytesAvailable() < (int)sizeof(quint32)) {
        if(!socket->isValid()) { // stale request
            return;
        }
        socket->waitForReadyRead(1000);
    }
    QDataStream ds(socket);
    QByteArray uMsg;
    quint32 remaining;
    ds >> remaining;
    uMsg.resize(remaining);
    int got = 0;
    char* uMsgBuf = uMsg.data();
    do {
        got = ds.readRawData(uMsgBuf, remaining);
        remaining -= got;
        uMsgBuf += got;
    } while (remaining && got >= 0 && socket->waitForReadyRead(2000));
    if (got < 0) {
        qWarning("QtLocalPeer: Message reception failed %s", socket->errorString().toLatin1().constData());
        delete socket;
        return;
    }
    QString message(QString::fromUtf8(uMsg));
    socket->write(ack, qstrlen(ack));
    socket->waitForBytesWritten(1000);
    socket->waitForDisconnected(1000); // make sure client reads ack
    delete socket;
    emit messageReceived(message); //### (might take a long time to return)
}
