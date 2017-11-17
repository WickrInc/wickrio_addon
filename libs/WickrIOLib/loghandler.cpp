#include <QCoreApplication>
#include <QDateTime>

#include "loghandler.h"
#include "wickrbotutils.h"

LogHandler::LogHandler() :

    m_pid(QCoreApplication::applicationPid()),
    m_wbLog(NULL)
{

}


LogHandler::~LogHandler()
{
    if (m_wbLog != NULL) {
        delete m_wbLog;
        m_wbLog = NULL;
    }

    QCoreApplication::processEvents();
}


void LogHandler::setupLog(const QString &logFileName)
{
    m_wbLog = new WickrBotLog(logFileName);
}


QDateTime LogHandler::lastLogTime()
{
    if(m_wbLog != NULL)
        return m_wbLog->lastLogTime();
    //return invalid date if log not set
    return QDateTime();
}

QString LogHandler::getLogFile()
{
    if (m_wbLog == NULL)
        return QString("");
    return m_wbLog->getFileName();
}

