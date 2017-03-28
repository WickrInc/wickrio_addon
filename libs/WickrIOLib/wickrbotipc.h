#ifndef WICKRBOTIPC_H
#define WICKRBOTIPC_H

#include <QObject>
#include <QtNetwork>

#include "wickrbotlib.h"

class DECLSPEC WickrBotIPC : public QObject
{
    Q_OBJECT
public:
    explicit WickrBotIPC(QObject *parent = 0);

    // Server functions
    bool startServer();
    int getServerPort();

    // Client functions
    bool sendMessage(int ipc_port, const QString &message);
    QByteArray getResponse(int ipc_port, bool *result);

    bool check();

signals:
    void signalGotMessage(const QString &message, int peerPort);
    void signalSentMessage();
    void signalSendError();

public slots:
    void slotNewConnection();
    void slotReadyToSend();
    void slotErrorSend(QAbstractSocket::SocketError socketError);

private:
    QTcpServer *m_server;
    QTcpSocket *m_socket;

    QString m_sendMessage;
};

#endif // WICKRBOTIPC_H
