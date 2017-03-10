#ifndef PERFTEST_H
#define PERFTEST_H

#include <QObject>
#include <QElapsedTimer>

#include "wickrbotlib.h"

class DECLSPEC PerfTest : public QObject
{
    Q_OBJECT
public:
    PerfTest(const QString & name="");
    ~PerfTest();

    void start(const QString & name="");
    void stop();
    void print();

signals:

public slots:

private:
    QElapsedTimer m_timer;
    int m_instances;
    long m_elapsedSum;
    bool m_timerStarted;

public:
    QString m_name;
};

#endif // PERFTEST_H
