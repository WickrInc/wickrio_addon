#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QSettings>

#include <wickrbotutils.h>
#include "wickrbotsettings.h"
#include "wickrioclientserverprocess.h"
#include "wickrIOCommon.h"
#include "wickrIOServerCommon.h"

WickrIOClientServerProcess *WickrIOClientServerProcess::theClientServer;

//#define DEBUG_TRACE 1

/**
 * @brief WickrIOClientServerProcess::WickrIOClientServerProcess
 * This is the constructor for the WickrIO service (windows) / Daemon (linux)
 */
WickrIOClientServerProcess::WickrIOClientServerProcess()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering constructor";
#endif

    connect(this, &WickrIOClientServerProcess::started, this, &WickrIOClientServerProcess::processStarted);
    connect(this, &WickrIOClientServerProcess::finished, this, &WickrIOClientServerProcess::processFinished);

    m_clientServer.m_operation = new OperationData();
    m_clientServer.m_operation->processName = WBIO_CLIENTSERVER_TARGET;

    if (m_clientServer.configureService()) {
        m_clientServer.m_operation->updateProcessState(PROCSTATE_RUNNING);
    }
#ifdef DEBUG_TRACE
    qDebug() << "Leaving constructor";
#endif
}

WickrIOClientServerProcess::~WickrIOClientServerProcess()
{
#ifdef DEBUG_TRACE
    qDebug() << "Entering destructor";
#endif
    m_clientServer.m_operation->updateProcessState(PROCSTATE_DOWN);
#ifdef DEBUG_TRACE
    qDebug() << "Leaving destructor";
#endif
}

void
WickrIOClientServerProcess::processStarted()
{
    m_clientServer.processStarted();
}

void
WickrIOClientServerProcess::processFinished()
{
    m_clientServer.processFinished();
}
