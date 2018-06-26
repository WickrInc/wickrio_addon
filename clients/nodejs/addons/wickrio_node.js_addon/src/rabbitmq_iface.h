#ifndef RABBITMQ_TEST_H
#define RABBITMQ_TEST_H

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif

#include <unistd.h>
#include "AMQPcpp.h"

using namespace std;

class RabbitMQIface
{
public:
    RabbitMQIface(string client);
    ~RabbitMQIface();

    bool init(const string& amqp);
    string sendMessage(string message);

private:
    AMQP            *m_amqp = nullptr;
    AMQPQueue       *m_responseQueue = nullptr;
    AMQPExchange    *m_exch = nullptr;

    string  m_clientName;
    string  m_requestQName;
    string  m_responseQName;


    static string m_responseString;

    static int onCancel(AMQPMessage * message);
    static int onMessage(AMQPMessage * message);

};

#endif // RABBITMQ_TEST_H
