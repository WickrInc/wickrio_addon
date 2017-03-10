#ifndef WICKRBOTLOG_H
#define WICKRBOTLOG_H

#include <QFile>
#include <QString>
#include <QFileInfo>
#include "wickrbotlib.h"

class DECLSPEC WickrBotLog
{
public:
    WickrBotLog(QString logFileName);
    ~WickrBotLog();

    typedef enum WickrBotLogType { NormalLog, ErrorLog, StatisticLog, RedirectedOutput } WickrBotLogType_t;

    void write(const QString& message, WickrBotLogType type=NormalLog);
    void error(const QString& message) { write(message, ErrorLog); }
    void write(const QString& message, int count, WickrBotLogType type=StatisticLog);

    QString getFileName() { return m_fileName; }

    QDateTime lastLogTime();

    void setOutputFilename(const QString& outFilename) { m_outFileName = outFilename; }

private:
    QString toString(WickrBotLogType type) {
        if (type == NormalLog)
            return "LOG";
        if (type == ErrorLog)
            return "ERROR";
        if (type == StatisticLog)
            return "STATISTIC";
        return "LOG";
    }
    void writeToFile(const QString& filename, const QString& message);

private:
    QString m_fileName;
    long m_pid;

    // Set when stdout is being redirected
    QString m_outFileName;
};

#define LOGS_DATETIME_FORMAT      "yyyy-MM-dd hh:mm:ss"
#define LOGS_DEFAULT_MAXFILESIZE    10000000
#define LOGS_DEFAULT_NUMFILES       5


#endif // WICKRBOTLOG_H
