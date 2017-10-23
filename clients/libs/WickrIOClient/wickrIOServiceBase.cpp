#include "wickrIOServiceBase.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOWatchdogService.h"

WickrIOServiceBase::WickrIOServiceBase(const QString& serviceName) :
    m_serviceName(serviceName)
{

}

WickrIOServiceBase::~WickrIOServiceBase()
{
    stopHeartBeat();
}

/**
 * @brief WickrIOServiceBase::startHeartBeat
 * This function will start the heart beat process.  It will create a timer
 * and a connection to perform heartbeat processing when that timer expires.
 */
void
WickrIOServiceBase::startHeartBeat()
{
    // Register this service with the watchdog service
    WickrIOClientRuntime::wdSvc()->registerService(this);

    // Start the timer
    if (!m_timer.isActive()) {
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
        m_timer.start(1000 * WICKRIO_WATCHDOG_UPDATE_PROCESS_SECS);
    }
}

/**
 * @brief WickrIOServiceBase::stopHeartBeat
 * This function will stop the heartbeat process.  If the timer is active it will
 * be started.  The connection to the timer's timeout signal will be disconnected.
 */
void
WickrIOServiceBase::stopHeartBeat()
{
    // DeRegister this service with the watchdog service
    WickrIOClientRuntime::wdSvc()->deRegisterService(this);

    if (m_timer.isActive())
        m_timer.stop();
    disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotTimerExpire()));
}

/**
 * @brief WickrIOServiceBase::slotTimerExpire
 * WHen the heartbeat timer expires update the heartbeat value and emit a signal.
 * the signal should be consumed by the watchdog service.
 */
void
WickrIOServiceBase::slotTimerExpire()
{
    QDateTime asd(QDateTime::currentDateTime());
    m_heartbeat = asd.toTime_t();

    emit signalHeartBeat(this);
}
