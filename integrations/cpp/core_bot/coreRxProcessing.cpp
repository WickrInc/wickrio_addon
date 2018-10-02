#include <QDebug>
#include "coreRxProcessing.h"

CoreRxProcessing::CoreRxProcessing(CoreOperationData *operation, QObject *parent) :
    QObject(parent),
    m_operation(operation)
{
}

CoreRxProcessing::~CoreRxProcessing()
{
}

bool
CoreRxProcessing::timerCall()
{
    if (!m_operation->m_botIface)
        return false;

    // get messages off the receive queue
    string command;
    m_operation->m_botIface->cmdStringGetReceivedMessage(command);

    bool done=false;
    while (!done) {
        string response;
        if (m_operation->m_botIface->send(command, response) != BotIface::SUCCESS) {
            qDebug() << QString("Send failed: %1").arg(QString(m_operation->m_botIface->getLastErrorString().c_str()));
            return false;
        }

        if (response.length() == 0)
            done=true;
        else {
            qDebug() << "Got message";
        }
    }
    return true;
}
