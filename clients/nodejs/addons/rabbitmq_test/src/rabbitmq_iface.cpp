#include "rabbitmq_iface.h"

using namespace std;

string RabbitMQIface::m_responseString;

int
RabbitMQIface::onCancel(AMQPMessage * message)
{
    cout << "cancel tag="<< message->getDeliveryTag() << endl;
    return 0;
}

int
RabbitMQIface::onMessage( AMQPMessage * message ) {
    uint32_t i = 0;
    char * data = message->getMessage(&i);

    AMQPQueue * q = message->getQueue();
    q->Ack( message->getDeliveryTag() );
//    q->Cancel( message->getConsumerTag() );

#if 0
    i++;

    cout << "#" << i << " tag="<< message->getDeliveryTag() << " content-type:"<< message->getHeader("Content-type") ;
    cout << " encoding:"<< message->getHeader("Content-encoding")<< " mode="<<message->getHeader("Delivery-mode")<<endl;

    if (i > 10) {
        AMQPQueue * q = message->getQueue();
        q->Cancel( message->getConsumerTag() );
    }
#endif

    if (data) {
        string rspStr(data);
        RabbitMQIface::m_responseString = rspStr;
    } else {
        RabbitMQIface::m_responseString = "";
    }
    return 1;
}

RabbitMQIface::RabbitMQIface(string client) :
    m_clientName(client)
{
}

RabbitMQIface::~RabbitMQIface()
{
    if (m_amqp) {
        delete m_amqp;
    }
}

bool
RabbitMQIface::init()
{
    m_amqp = new AMQP("guest:guest@localhost:5672");

    long randValue = rand();
    string randValueString = to_string(randValue);
    string responseQueue = "rcv_" + randValueString;

    m_requestQName = m_clientName + "_rpc";
    m_responseQName = responseQueue;
//    m_requestQName = "rpc_queue";
//    m_responseQName = "response_queue";

    m_responseQueue = m_amqp->createQueue(m_responseQName);
    m_responseQueue->Declare(m_responseQueue->getName(), AMQP_EXCLUSIVE | AMQP_AUTODELETE);
    m_responseQueue->addEvent(AMQP_MESSAGE, onMessage );
    m_responseQueue->addEvent(AMQP_CANCEL, onCancel );

    m_exch = m_amqp->createExchange();
    return true;
}

string
RabbitMQIface::sendMessage(string message)
{
    long randValue = rand();
    string randValueString = to_string(randValue);

    m_exch->setHeader("Reply-to", m_responseQueue->getName());
    m_exch->setHeader("correlation_id", randValueString);

    m_exch->Publish(message , m_requestQName); // publish very long message

    string consumerTag = "tag_" + randValueString;
    m_responseQueue->setConsumerTag(consumerTag);

//    m_responseQueue->Consume(AMQP_NOACK);
    m_responseQueue->Consume();

    return RabbitMQIface::m_responseString;
}

