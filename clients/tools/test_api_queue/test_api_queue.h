#ifndef TEST_API_QUEUE_H
#define TEST_API_QUEUE_H

#include <QObject>

class QAmqpQueue;
class QAmqpExchange;
class QAmqpClient;

class TestAPIQueue : public QObject
{
    Q_OBJECT
public:
    explicit TestAPIQueue(QObject *parent = 0);
    ~TestAPIQueue();

    void start() { emit signalReady(); }
    void call(const QString& request);
    QString getResponse() { return m_lastReponse; }

private:
    QAmqpClient *m_client;
    QAmqpQueue *m_responseQueue;
    QAmqpExchange *m_defaultExchange;
    QString m_correlationId;
    QString m_lastReponse;

signals:
    void connected();
    void signalGotResponse();
    void signalReady();

public slots:
    bool connectToServer();

private slots:
    void clientConnected();
    void queueDeclared();
    void responseReceived();

};

#endif  // TEST_API_QUEUE_H
