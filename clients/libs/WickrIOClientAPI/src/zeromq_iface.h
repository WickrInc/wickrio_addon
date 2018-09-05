#ifndef ZEROMQ_TEST_H
#define ZEROMQ_TEST_H

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif

#include <unistd.h>
#include "zmq.h"

using namespace std;

class MesgQueueIface
{
public:
    MesgQueueIface(string client) : m_clientName(client) {}
    ~MesgQueueIface();

    bool init();
    string sendMessage(string message);

private:
    string      m_clientName;

    void        *m_zctx = NULL;
    void        *m_zTxSocket = NULL;
    void        *m_zRxSocket = NULL;

    string  m_responseString;

    string  m_requestQName;     // Socket to send requests on
    string  m_asyncQName;       // Socket to receive asynch events

    bool    m_doReceive = true; // true when receiving
    bool    m_rxThreadDone;     // helps during shutdown

    void    *m_rxThread = NULL;

    void rxThread();
};

#endif // ZEROMQ_TEST_H
