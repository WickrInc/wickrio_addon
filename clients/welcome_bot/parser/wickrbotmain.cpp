#include <QDebug>

#include "wickrbotmain.h"
#include "parseroperationdata.h"

WickrBotMain *WickrBotMain::theBot;

WickrBotMain::WickrBotMain(ParserOperationData *operation) :
    m_operation(operation),
    m_qamqp(NULL),
    m_logcountdown(LOG_COUNTDOWN),
    m_seccount(0),
    m_qfailures(0)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(doTimerWork()));
    timer.start(1000);
}

WickrBotMain::~WickrBotMain()
{
    timer.stop();
    timer.deleteLater();
}

void WickrBotMain::doTimerWork()
{
    m_seccount++;

    if (--m_logcountdown <= 0) {
        m_operation->log_handler->log("Keep alive message");
        m_operation->updateProcessState(PROCSTATE_RUNNING);
        m_logcountdown = LOG_COUNTDOWN;
    }

    // If we have reached the limit of how long to run then log and exit
    if (m_operation->duration && m_seccount > m_operation->duration) {
        m_operation->log_handler->log("Duration time has been reached, exiting!");
        m_operation->updateProcessState(PROCSTATE_DOWN);
        QCoreApplication::exit(1);
    }


    if (m_qamqp == NULL) {
        m_qamqp = new WBParse_QAMQPQueue(m_operation);
    } else {
        // If the Queue handler fails then delete the queue handler.
        // It will be recreated on the next iteration.
        if (! m_qamqp->timerCall()) {
            m_operation->log_handler->log("Message queue handler failed.  Deleting handler.");
            delete  m_qamqp;
            m_qamqp = NULL;

            m_qfailures++;
        } else if (! m_qamqp->isRunning()) {
            m_qfailures++;
        } else {
            m_qfailures = 0;
        }

        // If more than 5 Qfailures have occured then exit.
        // Hopefully a restart will fix the problem
        if (m_qfailures > 5) {
            m_operation->log_handler->log("More than 5 successive queue failures, exiting!");
            m_operation->updateProcessState(PROCSTATE_DOWN);
            QCoreApplication::exit(1);
        }
    }
}

