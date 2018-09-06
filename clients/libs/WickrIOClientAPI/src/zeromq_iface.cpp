#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>

#include "zeromq_iface.h"

using namespace std;

MesgQueueIface::~MesgQueueIface()
{
    if (m_doReceive) {
        m_rxThreadDone = false;
        m_doReceive = false;

        while (!m_rxThreadDone) {
            std::cout << "Waiting for rx thread to finish!\n";
            sleep(1);
        }
        std::cout << "Finished waiting for rx thread!\n";
    }
}

void
MesgQueueIface::rxThread()
{
    while (m_doReceive) {
        /* Create an empty ØMQ message */
        zmq_msg_t reply;
        int rc = zmq_msg_init (&reply);
        if (rc != 0) {
            const char *errstring = zmq_strerror(zmq_errno());
            std::cout << "Error initializing message: " << errstring << "\n";
            return;
        }

        rc = zmq_recvmsg(m_zRxSocket, &reply, ZMQ_DONTWAIT);
        if (rc == -1) {
            // No messages on the queue
            if (zmq_errno() == EAGAIN) {
                continue;
            }
            const char *errstring = zmq_strerror(zmq_errno());
            std::cout << "Error receiving: " << errstring << "\n";
            return;
        }

        string tmp((char *)zmq_msg_data(&reply), zmq_msg_size(&reply));
        m_responseString = tmp;

        std::cout << "Got Message:" << MesgQueueIface::m_responseString;



        /* Create a new message, allocating 6 bytes for message content */
        zmq_msg_t msg;
        string message="message received";
        rc = zmq_msg_init_size (&msg, message.length());
        if (rc != 0) {
            const char *errstring = zmq_strerror(rc);
            std::cout << "Error initializing message: " << errstring << "\n";
            return;
        }

        memcpy(zmq_msg_data(&msg), message.c_str(), message.length());
        /* Send the message to the socket */
        rc = zmq_sendmsg(m_zRxSocket, &msg, 0);
        if (rc == -1) {
            const char *errstring = zmq_strerror(zmq_errno());
            std::cout << "Error sending: " << errstring << "\n";
            return;
        }

    }
    std::cout << "Rx Thread is finished!\n";
    m_rxThreadDone = true;
}

bool
MesgQueueIface::init()
{
#ifdef WICKR_PRODUCTION
    m_requestQName = "ipc:///opt/WickrIO/clients/" + m_clientName + "/tmp/0";
    m_asyncQName = "ipc:///opt/WickrIO/clients/" + m_clientName + "/tmp/2";
#else
    m_requestQName = "ipc:///opt/WickrIODebug/clients/" + m_clientName + "/tmp/0";
    m_asyncQName = "ipc:///opt/WickrIODebug/clients/" + m_clientName + "/tmp/2";
#endif

    //  Prepare our context and socket
    m_zctx = zmq_ctx_new();

    {
        m_zTxSocket = zmq_socket(m_zctx, ZMQ_REQ);
        int rc = zmq_connect(m_zTxSocket, m_requestQName.c_str());
        if (rc != 0) {
            const char *errstring = zmq_strerror(rc);
            std::cout << "Error connecting to socket: " << errstring << "\n";
            return false;
        }
    }

    {
        m_zRxSocket = zmq_socket(m_zctx, ZMQ_REP);
        int rc = zmq_connect(m_zRxSocket, m_asyncQName.c_str());
        if (rc != 0) {
            const char *errstring = zmq_strerror(rc);
            std::cout << "Error connecting to socket: " << errstring << "\n";
            return false;
        }
    }

    // Create a receive thread
    m_rxThread = new std::thread(&MesgQueueIface::rxThread, this);

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

