#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include <QObject>

#include "wickrbotlib.h"
#include "wickrbotlog.h"

class DECLSPEC LogHandler : public QObject
{
Q_OBJECT
public:
    LogHandler();
    ~LogHandler();

public:
    int m_pid;          // The process ID of this application


private:
    WickrBotLog *m_wbLog;

public:
    void setupLog(const QString &logFileName);
    QString getLogFile();

    void error(QString message) {
        if (m_wbLog != NULL)
            m_wbLog->error(message);
    }

    void log(QString message) {
        if (m_wbLog != NULL)
            m_wbLog->write(message);
    }

    void log(QString message, int count) {
        if (m_wbLog != NULL)
            m_wbLog->write(message, count);
    }

    void output(QString message) {
        if (m_wbLog != NULL)
            m_wbLog->write(message, WickrBotLog::RedirectedOutput);
    }

    void logSetOutput(const QString& outputFilename) { if (m_wbLog) m_wbLog->setOutputFilename(outputFilename); }
    QString logGetOutput() { return m_wbLog ? m_wbLog->getOutputFilename() : QString(); }

    QDateTime lastLogTime();

};

#endif // LOGHANDLER_H
