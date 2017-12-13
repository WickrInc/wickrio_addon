#ifndef WICKRIOTHREAD_H
#define WICKRIOTHREAD_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QDebug>

typedef enum { Idle, Running, Stopping, Exiting } WickrIOThreadState;

class WickrIOThread : public QThread
{
    Q_OBJECT
public:
    WickrIOThread();
    virtual ~WickrIOThread() {}

    Q_INVOKABLE void stopProcessing() {
        qDebug() << "in stopProcessing()!";
        if (m_threadState != Idle) {
            m_threadState = Stopping;
            m_shuttingdown = true;
            stopTimer();
            if (!m_processing) {
                makeIdle();
            }
        } else {
            emit signalStopped();
        }
    }

    Q_INVOKABLE void startProcessing() {
        m_shuttingdown = false;
        if (m_threadState != Running) {
            startTimer();
            m_threadState = Running;
        }
    }

signals:
    void signalStopped();

private slots:
    void slotDoTimerWork();
    void slotProcessStarted();

protected:
    void startTimer();
    void stopTimer();
    void makeIdle() {
        m_threadState = Idle;
        emit signalStopped();
    }

    virtual void processStarted() {}
    virtual void onTimerAction() {}

    QTimer timer;
    bool m_processing;          // True when processing
    bool m_shuttingdown;

public:
    WickrIOThreadState m_threadState;

};

#endif // WICKRIOTHREAD_H
