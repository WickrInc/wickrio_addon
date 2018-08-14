#include "wickrIOConsole.h"
#include "cmdMain.h"

WickrIOConsoleService::WickrIOConsoleService(QCoreApplication *app, int argc, char **argv, OperationData *operation, WickrIOIPCService *ipcSvc) :
    m_app(app),
    m_argc(argc),
    m_argv(argv),
    m_operation(operation),
    m_ipcSvc(ipcSvc)
{
    // Start threads
    startThreads();

    setObjectName("WickrIOConsoleThread");
    qDebug() << "WICKRIOCONSOLE SERVICE: Started.";
}

WickrIOConsoleService::~WickrIOConsoleService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOCONSOLE SERVICE: Shutdown.";
}

/**
 * @brief WickrIOConsoleService::startThreads
 * Starts all threads on message service.
 */
void WickrIOConsoleService::startThreads()
{
    // Allocate threads
    m_consoleThread = new WickrIOConsoleThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOConsoleService::signalStartConsole,
            m_consoleThread, &WickrIOConsoleThread::slotStartConsole, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOConsoleService::stopThreads
 * Stops all threads on switchboard service.
 */
void WickrIOConsoleService::stopThreads()
{
    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("WICKRIOCONSOLE THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOConsoleService::startConsole()
{
    emit signalStartConsole();
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

WickrIOConsoleThread::WickrIOConsoleThread(QThread *thread, WickrIOConsoleService *consoleSvc) :
    m_parent(consoleSvc),
    m_running(false)
{
    thread->setObjectName("WickrIOFileDownloadThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    m_running = true;
}

/**
 * @brief WickrIOConsoleThread::~WickrIOConsoleThread
 * Destructor
 */
WickrIOConsoleThread::~WickrIOConsoleThread() {
    qDebug() << "WICKRIOCONSOLE THREAD: Worker Destroyed.";
}

void
WickrIOConsoleThread::slotStartConsole()
{
    CmdMain mainCommands(m_parent->m_app, m_parent->m_argc, m_parent->m_argv, m_parent->m_operation, m_parent->m_ipcSvc);

    connect(m_parent->m_ipcSvc, &WickrIOIPCService::signalReceivedMessage, &mainCommands, &CmdMain::signalReceivedMessage);

    mainCommands.runCommands();

    m_parent->m_app->quit();
}
