#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>

#include "wickrbotutils.h"

WickrBotUtils::WickrBotUtils()
{
}

QString WickrBotUtils::fileInList(const QString &filename, const QStringList &searchList)
{
    QString fullPath("");

    foreach (QString dir, searchList) {
        QFile file(dir+"/"+filename);
        if (file.exists()) {
            // found
            fullPath=QDir(file.fileName()).canonicalPath();
            qDebug("Found file %s",qPrintable(fullPath));
            break;
        }
    }
    return fullPath;
}

/**
 * @brief WickrBotUtils::isRunning
 * This function will check if the input process is running.
 * @param process
 * @return
 */
bool WickrBotUtils::isRunning(const QString &appName, int pid)
{
    QProcess exec;

#ifdef Q_OS_WIN
    exec.start("tasklist",
               QStringList() << "/NH"
                             << "/FO" << "CSV"
                             << "/FI" << QString("PID eq %1").arg(pid));
    exec.waitForFinished();
    QString pstdout = exec.readAllStandardOutput();
    if (pstdout.contains(appName)) {
        return true;
    }
#else
    // Else it has been longer than 10 minutes, kill the old process and continue
    QStringList arguments;
    QString command = QString("ps -a -o pid,command | grep %1 | grep %2").arg(appName).arg(pid);
    arguments << "-c" << command;
    exec.start("/bin/sh", arguments);
    exec.waitForFinished(-1);
    QString pstdout = exec.readAllStandardOutput();

    // If the command returned a string then this should be the correct process to kill
    if (pstdout.length()) {
        return true;
    }
#endif
    return false;
}

/**
 * @brief WickrBotUtils::killProcess
 * This function will kill the process with the input Process ID.
 * TODO: Need to add ability to send signal to stop process gracefully!
 * @param pid
 */
void WickrBotUtils::killProcess(int pid)
{
    QProcess exec;
#ifdef Q_OS_WIN
    QString command = QString("taskkill /f /t /pid %1").arg(pid);
#else
    QString command = QString("kill -9 %1").arg(pid);
#endif
    QStringList killargs;
    killargs << "-c" << command;
    exec.start("/bin/sh", killargs);
    exec.waitForFinished(-1);
}
