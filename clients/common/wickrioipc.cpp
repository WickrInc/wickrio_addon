#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "wickrioipc.h"
#include "wickriojson.h"
#include "wickrbotprocessstate.h"

#include "user/wickrUser.h"
#include "libinterface/libwickrcore.h"
#include "messaging/wickrInbox.h"
#include "wbio_common.h"

WickrBotMainIPC *WickrBotMainIPC::theBotIPC;

/**
 * @brief WickrBotMainIPC::WickrBotMainIPC
 * This is the constructor for this class. Variables are initialized, the logins are created and
 * the m_logins list is set with those logins. Logging is started, counts initialized and SLOTS
 * are setup to receive specific SIGNALs
 */
WickrBotMainIPC::WickrBotMainIPC(OperationData *operation) :
    m_operation(operation)
{
    this->connect(this, &WickrBotMainIPC::started, this, &WickrBotMainIPC::processStarted);
}

/**
 * @brief WickrBotMainIPC::~WickrBotMainIPC
 * This is the destructor for this class. The timer is stopped and the DB closed.
 */
WickrBotMainIPC::~WickrBotMainIPC()
{
}

bool WickrBotMainIPC::check()
{
    if (m_ipc != NULL) {
        return m_ipc->check();
    }
    return true;
}

void WickrBotMainIPC::processStarted()
{
    m_operation->log("Started WickrBotMainIPC");

    m_ipc = new WickrBotIPC();
    m_ipc->startServer();

    m_operation->log(QString("server port=%1").arg(m_ipc->getServerPort()));

    if (m_operation->m_botDB != NULL && m_operation->m_botDB->isOpen()) {
        m_operation->m_botDB->setProcessIPC(m_operation->processName, m_ipc->getServerPort());
    } else {
        m_operation->log("WickrBotMainIPC: database is not open yet!");
    }

    m_operation->log("WickrBotMainIPC: processStartedA");
    QObject::connect(m_ipc, &WickrBotIPC::signalGotMessage, [=](const QString &message, int peerPort) {
        Q_UNUSED(peerPort);
        if (message == WBIO_IPCCMDS_STOP) {
            m_operation->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_STOP));
            m_operation->log("WickrBotMainIPC::processStarted: QUITTING");
            emit signalGotStopRequest();
        } else if (message == WBIO_IPCCMDS_PAUSE) {
            m_operation->log(QString("GOT MESSAGE: %1").arg(WBIO_IPCCMDS_PAUSE));
            m_operation->log("WickrBotMainIPC::processStarted: PAUSING");
            emit signalGotPauseRequest();
        } else {
            QStringList pieces = message.split("=");
            if (pieces.size() == 2) {
                QString type = pieces.at(0);
                QString value = pieces.at(1);
                m_operation->log(QString("GOT MESSAGE: %1").arg(type));
                emit signalReceivedMessage(type, value);
            } else {
                m_operation->log("GOT MESSAGE: invalid message:" + message);
            }
        }
    });
    m_operation->log("WickrBotMainIPC: processStartedB");
}

/**
 * @brief WickrBotMainIPC::stopAndExit
 * This is a SLOT that is called if there is a failure during the startup of the application.
 */
void WickrBotMainIPC::stopAndExit()
{
}

