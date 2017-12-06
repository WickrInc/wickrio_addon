#include <QJsonDocument>

#include "wickrIOWatchdogService.h"
#include "wickrIOClientRuntime.h"

#include "common/wickrRuntime.h"
#include "filetransfer/wickrFileInfo.h"
#include "filetransfer/wickrCloudTransferMgr.h"
#include "wickriodatabase.h"


WickrIOWatchdogService::WickrIOWatchdogService() :
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_wdThread(nullptr),
    m_shutdownMode(PROCSTATE_PAUSED)    // Default shutdown proc state is Paused
{
    qRegisterMetaType<WickrServiceState>("WickrServiceState");
    qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

    // Start threads
    startThreads();

    setObjectName("WickrIOWatchdogThread");
    qDebug() << "WICKRIOWATCHDOG SERVICE: Started.";
    m_state = WickrServiceState::SERVICE_STARTED;
}

WickrIOWatchdogService::~WickrIOWatchdogService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOWATCHDOG SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOWatchdogService::startThreads
 * Starts all threads on message service.
 */
void WickrIOWatchdogService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_wdThread = new WickrIOWatchdogThread(&m_thread, this);

    // Emit signals from the watchdog thread
    connect(m_wdThread, &WickrIOWatchdogThread::signalServiceNotLoggedIn,
            this, &WickrIOWatchdogService::signalServiceNotLoggedIn);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOWatchdogService::stopThreads
 * Stops all threads on switchboard service.
 */
void WickrIOWatchdogService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOWATCHDOG THREAD: Shutdown Thread (%p)", &m_thread);
}

/**
 * @brief WickrIOWatchdogService::shutdown
 * Called to shutdown the watchdog service.
 */
void WickrIOWatchdogService::shutdown()
{
    QEventLoop wait_loop;
    connect(m_wdThread, &WickrIOWatchdogThread::signalNotRunning, &wait_loop, &QEventLoop::quit);
    emit signalShutdown(m_shutdownMode);

    qDebug() << "Watchdog Service shutdown: starting to wait";
    // Wait for the thread to signal it is done
    wait_loop.exec();
    qDebug() << "Watchdog Service shutdown: finished waiting";
}

void WickrIOWatchdogService::registerService(WickrIOServiceBase *svc)
{
    emit signalRegisterService(svc);
}

void WickrIOWatchdogService::deRegisterService(WickrIOServiceBase *svc)
{
    emit signalDeRegisterService(svc);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


WickrIOWatchdogThread::WickrIOWatchdogThread(QThread *thread, WickrIOWatchdogService *wdSvc) :
    m_lock(QReadWriteLock::Recursive),
    m_parent(wdSvc),
    m_running(false),
    m_operation(nullptr)
{
    thread->setObjectName("WickrIOWatchdogThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    // Set the initial status
    doStatusUpdate(PROCSTATE_RUNNING, true);

    // Start the timer
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
    m_timer.start(1000 * WICKRIO_WATCHDOG_UPDATE_PROCESS_SECS);

    // Catch the shutdown signal
    connect(wdSvc, &WickrIOWatchdogService::signalShutdown, this, &WickrIOWatchdogThread::slotShutdown);

    // Catch the register and deregister signals
    connect(wdSvc, &WickrIOWatchdogService::signalRegisterService, this, &WickrIOWatchdogThread::slotRegisterService);
    connect(wdSvc, &WickrIOWatchdogService::signalDeRegisterService, this, &WickrIOWatchdogThread::slotDeRegisterService);

    m_running = true;
}

/**
 * @brief WickrIOWatchdogThread::~WickrIOWatchdogThread
 * Destructor
 */
WickrIOWatchdogThread::~WickrIOWatchdogThread() {
    m_running = false;

    // Stop the timer
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));

    qDebug() << "WICKRIOWATCHDOG THREAD: Worker Destroyed.";
}

void
WickrIOWatchdogThread::slotTimerExpire()
{
    if (m_running) {
        doStatusUpdate(PROCSTATE_RUNNING, true);

        // Check the Switchboard thread state
        WickrSwitchboardService* swbSvc = WickrCore::WickrRuntime::swbSvc();
        if (swbSvc != nullptr) {
            if (swbSvc->downSeconds() > 60) {
                qDebug() << "The SwitchBoard system is down for more than 60 seconds";
                emit signalServiceNotLoggedIn();
            }
        }
    }
}

void
WickrIOWatchdogThread::doStatusUpdate(int state, bool force)
{
    if (m_running) {
        if (m_operation == nullptr) {
            m_operation = WickrIOClientRuntime::operationData();
        }

        // Update the process status
        if (m_operation != nullptr) {
            m_operation->updateProcessState(state, force);
        }
    }
}

void
WickrIOWatchdogThread::slotShutdown(int procState)
{
    if (m_timer.isActive())
        m_timer.stop();

    doStatusUpdate(procState, false);
    m_running = false;

    emit signalNotRunning();
}

void WickrIOWatchdogThread::slotRegisterService(WickrIOServiceBase *svc)
{

}

void WickrIOWatchdogThread::slotDeRegisterService(WickrIOServiceBase *svc)
{

}
