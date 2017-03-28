#include "wickriothread.h"

WickrIOThread::WickrIOThread() :
    m_processing(false),
    m_shuttingdown(false),
    m_threadState(Idle)
{
    this->connect(this, &WickrIOThread::started, this, &WickrIOThread::slotProcessStarted);
}

void WickrIOThread::slotProcessStarted()
{
    processStarted();
    startTimer();
}

// Timer definitions
void WickrIOThread::startTimer()
{
    m_threadState = Running;
    connect(&timer, SIGNAL(timeout()), this, SLOT(slotDoTimerWork()));
    timer.start(1000);
}

void WickrIOThread::stopTimer()
{
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(slotDoTimerWork()));
    if (timer.isActive())
        timer.stop();
}

void WickrIOThread::slotDoTimerWork()
{
    if (m_shuttingdown || m_threadState != Running) {
        if (!m_processing && m_threadState != Idle) {
            makeIdle();
        }
    } else {
        if (! m_processing) {
            onTimerAction();
        }
    }
}
