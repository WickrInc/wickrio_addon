#ifndef IPCSENDMESSAGE_H
#define IPCSENDMESSAGE_H

#include <QObject>

#include "nzmqt/nzmqt.hpp"

class IpcSendMessage : public QObject
{
    Q_OBJECT
public:
    explicit IpcSendMessage(QObject *parent = nullptr);

    void sendMessage(QString msg);

private:
    nzmqt::ZMQContext   *m_zctx = nullptr;
    nzmqt::ZMQSocket    *m_zsocket = nullptr;

    time_t  m_sentMessageTime;
    bool    m_messageSent=false;

signals:

public slots:
    void slotAsyncResponseReceived(const QList<QByteArray>& messages);
};

#endif // IPCSENDMESSAGE_H
