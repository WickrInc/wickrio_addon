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

    void        *m_zctx = nullptr;
    void        *m_zTxSocket = nullptr;
    void        *m_zRxSocket = nullptr;

    string m_responseString;

    string m_requestQName;

};

#endif // ZEROMQ_TEST_H
