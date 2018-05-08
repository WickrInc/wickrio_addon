#ifndef WICKRIOAPIIFACE_H
#define WICKRIOAPIIFACE_H

#include "qamqpclient.h"
#include "qamqpexchange.h"
#include "qamqpqueue.h"

class WickrIOAPIIface : public QObject
{
    Q_OBJECT
public:
    WickrIOAPIIface(QObject *parent = 0);
    ~WickrIOAPIIface();

    void makeRequest(const QString& request);

signals:
    void signalConnected();

public slots:
    bool slotConnectToServer();

private slots:
    void slotClientConnected();
    void slotQueueDeclared();
    void slotResponseReceived();

private:
    QAmqpClient     *m_client = nullptr;
    QAmqpQueue      *m_responseQueue = nullptr;
    QAmqpExchange   *m_defaultExchange = nullptr;
    QString         m_correlationId;

};

#endif // WICKRIOAPIIFACE_H
