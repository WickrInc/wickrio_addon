#include <iostream>
#include <cstring>

#include "zeromq_iface.h"

using namespace std;

MesgQueueIface::~MesgQueueIface()
{
}

bool
MesgQueueIface::init()
{
    m_requestQName = "ipc:///opt/WickrIODebug/clients/" + m_clientName + "/tmp/0";
//    m_requestQName = "tcp://localhost:4005";

    //  Prepare our context and socket
    m_zctx = zmq_ctx_new();
    m_zTxSocket = zmq_socket(m_zctx, ZMQ_REQ);
    int rc = zmq_connect(m_zTxSocket, m_requestQName.c_str());
    if (rc != 0) {
        const char *errstring = zmq_strerror(rc);
        std::cout << "Error connecting to socket: " << errstring << "\n";
        return false;
    }

    return true;
}

string
MesgQueueIface::sendMessage(string message)
{
    /* Create a new message, allocating 6 bytes for message content */
    zmq_msg_t msg;
    int rc = zmq_msg_init_size (&msg, message.length());
    if (rc != 0) {
        const char *errstring = zmq_strerror(rc);
        std::cout << "Error initializing message: " << errstring << "\n";
        return NULL;
    }

    memcpy(zmq_msg_data(&msg), message.c_str(), message.length());
    /* Send the message to the socket */
    rc = zmq_sendmsg(m_zTxSocket, &msg, 0);
    if (rc == -1) {
        const char *errstring = zmq_strerror(zmq_errno());
        std::cout << "Error sending: " << errstring << "\n";
        return NULL;
    }


    /* Create an empty ØMQ message */
    zmq_msg_t reply;
    rc = zmq_msg_init (&reply);
    if (rc != 0) {
        const char *errstring = zmq_strerror(zmq_errno());
        std::cout << "Error initializing message: " << errstring << "\n";
        return NULL;
    }

    rc = zmq_recvmsg(m_zTxSocket, &reply, 0);
    if (rc == -1) {
        const char *errstring = zmq_strerror(zmq_errno());
        std::cout << "Error receiving: " << errstring << "\n";
        return NULL;
    }

    string tmp((char *)zmq_msg_data(&reply), zmq_msg_size(&reply));
    m_responseString = tmp;

    return MesgQueueIface::m_responseString;
}

