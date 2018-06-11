#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "wickrioclientserverservice.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"

extern bool isVERSIONDEBUG();

//#define DEBUG_TRACE 1

/**
 * @brief WickrIOClientServerService::WickrIOClientServerService
 * This is the constructor for the WickrIO service (windows) / Daemon (linux)
 * @param argc
 * @param argv
 */
WickrIOClientServerService::WickrIOClientServerService(int argc, char **argv) :
    QtService<QCoreApplication>(argc, argv, WBIO_CLIENTSERVER_TARGET)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering constructor";
#endif

    setServiceDescription(QObject::tr("WickrIO Client Server Service"));
    setServiceFlags(QtServiceBase::CanBeSuspended);

    m_clientServer.m_operation = new OperationData();
    m_clientServer.m_operation->processName = WBIO_CLIENTSERVER_TARGET;

    if (m_clientServer.configureService()) {
        m_clientServer.m_operation->updateProcessState(PROCSTATE_RUNNING);
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving constructor";
#endif
}

WickrIOClientServerService::~WickrIOClientServerService()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering destructor";
#endif
    m_clientServer.m_operation->updateProcessState(PROCSTATE_DOWN);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving destructor";
#endif
}

/**
 * @brief WickrIOClientServerService::start
 * This is a service directive to start the service. This will start a timer that
 * will call the slotTimeoutProcess() slot on one second intervals, to perform the
 * tasks associated with this service.
 */
void WickrIOClientServerService::start()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering start";
#endif
    m_clientServer.processStarted();
#ifdef DEBUG_TRACE
    qDebug() << "Leaving start";
#endif
}

/**
 * @brief WickrIOClientServerService::stop
 * This is the stop service directive to stop this service. This directive will send
 * the WBIO_IPCCMDS_STOP IPC command to all of the active clients.
 */
void WickrIOClientServerService::stop()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering stop";
#endif
    m_clientServer.processFinished();
#ifdef DEBUG_TRACE
    qDebug() << "Leaving stop";
#endif
}

/**
 * @brief WickrIOClientServerService::pause
 * This is the pause service directive to pause this service. A WBIO_IPCCMDS_STOP IPC
 * command will be sent to all of the clients.
 */
void WickrIOClientServerService::pause()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering pause";
#endif
    m_clientServer.processFinished(true);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving pause";
#endif
}

/**
 * @brief WickrIOClientServerService::resume
 * This is the resume service directive to resume this service. The clients
 * will be restarted.
 */
void WickrIOClientServerService::resume()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering resume";
#endif
    m_clientServer.processStarted(true);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving resume";
#endif
}

/**
 * @brief WickrIOClientServerService::processCommand
 * @param code
 */
void WickrIOClientServerService::processCommand(int code)
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering processCommand";
#endif
    QString cmd = QString("received processCommand = %1").arg(code);
    qDebug() << "in processCommand:" << cmd;
#ifdef DEBUG_TRACE
    qDebug() << "Leaving processCommand";
#endif
}

