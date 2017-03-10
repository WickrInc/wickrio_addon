#include <QTextStream>
#include <QDateTime>
#include <QString>
#include <QCoreApplication>
#include <QDebug>

#include "wickrbotlog.h"

WickrBotLog::WickrBotLog(QString logFileName) :
    m_fileName(logFileName)
{
    m_pid = QCoreApplication::applicationPid();
}

WickrBotLog::~WickrBotLog()
{
}

QDateTime WickrBotLog::lastLogTime() {
    QFileInfo info(m_fileName);
    return info.lastModified();
}

void
WickrBotLog::write(const QString& message, WickrBotLogType type)
{
    QDateTime dt = QDateTime::currentDateTime();
    QString dateTime = dt.toString(LOGS_DATETIME_FORMAT);

    // Format the message
    QString outmessage = QString("%1, %2, %3, %4, %5,,\n")
            .arg(dateTime)
            .arg(toString(type))
            .arg(m_pid)
            .arg(QCoreApplication::applicationName())
            .arg(message);

    if (m_outFileName.isEmpty()) {
        qDebug() << toString(type) << outmessage;
    } else {
        writeToFile(m_outFileName, outmessage);
    }

    // If it is NOT redirected output then put into the log file
    if (type != RedirectedOutput) {
        outmessage = QString("%1(%2): %3\n").arg(toString(type)).arg(m_pid).arg(message);
        writeToFile(m_fileName, outmessage);
    }
}

void
WickrBotLog::write(const QString& message, int count, WickrBotLog::WickrBotLogType type)
{
    QDateTime dt = QDateTime::currentDateTime();
    QString dateTime = dt.toString(LOGS_DATETIME_FORMAT);

    // Format the message
    QString outmessage = QString("%1, %2, %3, %4, %5, %6,\n")
            .arg(dateTime)
            .arg(toString(type))
            .arg(m_pid)
            .arg(QCoreApplication::applicationName())
            .arg(message)
            .arg(count);

    if (m_outFileName.isEmpty()) {
        qDebug() << toString(type) << outmessage;
    } else {
        writeToFile(m_outFileName, outmessage);
    }

    // If it is NOT redirected output then put into the log file
    if (type != RedirectedOutput) {
        outmessage = QString("%1(%2): %3 %4\n").arg(toString(type)).arg(m_pid).arg(message).arg(count);
        writeToFile(m_fileName, outmessage);
    }
}

void
WickrBotLog::writeToFile(const QString& filename, const QString& message)
{
    QFile output(filename);
    // Only allow the log file to grow to a certain size
    if (output.size() >= LOGS_DEFAULT_MAXFILESIZE) {
        // If the max size is reached then rename the file, but only allow a certain number of files
        int curIndex = LOGS_DEFAULT_NUMFILES;
        do {
            QString savedFile = QString("%1.%2").arg(filename).arg(curIndex);
            QFile tmpFile(savedFile);
            if (tmpFile.exists())
                break;
        } while (--curIndex > 0);

        // Need to make space for the new file
        if (curIndex == LOGS_DEFAULT_NUMFILES) {
            for (int i=1; i<LOGS_DEFAULT_NUMFILES; i++) {
                QString fromFile = QString("%1.%2").arg(filename).arg(i+1);
                QString toFile   = QString("%1.%2").arg(filename).arg(i);
                QFile::remove(toFile);
                QFile tmpFile(fromFile);
                tmpFile.rename(toFile);
            }
            curIndex = LOGS_DEFAULT_NUMFILES;
        } else {
            curIndex++;
        }
        QString newFileName = QString("%1.%2").arg(filename).arg(curIndex);
        output.rename(newFileName);
        output.setFileName(filename);
    }

    output.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream outstream(&output);
    outstream << message;
    output.close();
}
