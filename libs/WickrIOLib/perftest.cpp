#include <QDebug>
#include "perftest.h"

/**
 * @brief PerfTest::PerfTest
 * Constructor for Performance testing class
 * @param name
 */
PerfTest::PerfTest(const QString & name) :
    m_instances(0),
    m_elapsedSum(0),
    m_timerStarted(false),
    m_name(name)
{
}

PerfTest::~PerfTest()
{
}

/**
 * @brief PerfTest::start
 * Start the performance test timer
 */
void
PerfTest::start(const QString & name)
{
    m_name = name;
    m_timerStarted = true;
    m_timer.start();
}

/**
 * @brief PerfTest::stop
 * Increment the elapsed sum and instances based on the current timer
 */
void
PerfTest::stop()
{
    int elapsed = m_timer.elapsed();
    m_elapsedSum += elapsed;
    m_instances++;
}

/**
 * @brief PerfTest::print
 * Print out the current average time for this instance
 */
void
PerfTest::print()
{
    if (m_elapsedSum > 0) {
        int avg = m_elapsedSum / m_instances;
        qDebug() << "Avg time for" << m_name << "is" << avg << "over" << m_instances << "instances";
    }
}
