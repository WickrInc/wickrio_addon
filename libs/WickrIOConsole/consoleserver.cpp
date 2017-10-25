#include <QProcess>
#include <QDebug>
#include <QString>
#include <QCoreApplication>

#include "consoleserver.h"
#include "wickrbotprocessstate.h"
#include "wickrIOCommon.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"

ConsoleServer::ConsoleServer(WickrIOClientDatabase *ioDB) :
    m_ioDB(ioDB)
{
    m_settings = WBIOServerCommon::getSettings();
}

bool
ConsoleServer::isRunning(const QString &processName, int timeout)
{
    WickrBotProcessState state;
    bool running = false;

    if (m_ioDB->getProcessState(processName, &state)) {
        // If the process is running then stop it
        if (state.state == PROCSTATE_RUNNING) {
            // Command to stop the Clients Server Daemon/Service
            running = true;

            const QDateTime dt = QDateTime::currentDateTime();
            int secs = state.last_update.secsTo(dt);

            // If greater than the timeout then check if the process is running
            if (secs > timeout) {
#if 0
                QString appName = QFileInfo(QCoreApplication::arguments().at(0)).fileName();
#else
                QString appName = "WickrIO";
#endif
                if (WickrBotUtils::isRunning(appName, state.process_id)) {
//                    WickrBotUtils::killProcess(state.process_id);
                    running = true;
                } else {
                    running = false;
                }
            }
        }
    }

    return running;
}

bool
ConsoleServer::runCommand(const QString &processName, const QString &action)
{
#ifdef Q_OS_LINUX
    QProcess exec;
    QString command = QString("systemctl %1 %2").arg(action).arg(processName);
    exec.start(command);
    if (exec.waitForStarted()) {
        QCoreApplication::processEvents();
        QString msg = QString("Waiting to %1 %2 service").arg(action).arg(processName);
        qDebug() << msg;
        while (! exec.waitForFinished(100)) {
            QCoreApplication::processEvents();
        }
        return true;
    } else {
        QString msg = QString("%1 service failed to %2").arg(processName).arg(action);
        qDebug() << msg;
        return false;
    }
#else
    return true;
#endif
}

void
ConsoleServer::setState(bool start, const QString &processName)
{
    QCoreApplication::processEvents();

#ifdef Q_OS_LINUX
    if (start) {
        if (runCommand(processName, "enable") && runCommand(processName, "start")) {
            qDebug().noquote().nospace() << "Started the " << processName << " service";
        } else {
            qDebug().noquote().nospace() << "Cound NOT start the " << processName << " service";
        }
    } else {
        if (runCommand(processName, "stop") && runCommand(processName, "disable")) {
            qDebug().noquote().nospace() << "Stopped the " << processName << " service";
        } else {
            qDebug().noquote().nospace() << "Cound NOT stop the " << processName << " service";
        }
    }

#elif defined(Q_OS_WIN)
    QProcess exec;
    QString command = QString("net %1 %2").arg(start ? "start" : "stop").arg(processName);

    #if 1
    exec.start(command);
    if (exec.waitForStarted()) {
        QCoreApplication::processEvents();
        QString msg = QString("%1 service %2").arg(processName).arg(start ? "started" : "stopped");
        qDebug() << msg;
        while (! exec.waitForFinished(100)) {
            QCoreApplication::processEvents();
        }
    } else {
        QString msg = QString("%1 service failed to %2").arg(processName).arg(start ? "start" : "stop");
        qDebug() << msg;
    }

    // Change the service to automatic if being started, or demand if stopped
    QProcess exec_sc;
    command = QString("sc config %1.exe start=%2").arg(processName).arg(start ? "auto" : "demand");

    exec_sc.start(command);
    if (exec_sc.waitForStarted()) {
        QCoreApplication::processEvents();
        while (! exec_sc.waitForFinished(100)) {
            QCoreApplication::processEvents();
        }
        qDebug() << "Successfully changed startup options for" << processName;
    } else {
        qDebug() << "Failed to change startup options for" << processName;
    }
    #else
    if (exec.startDetached(command)) {
        QString msg = QString("%1 service %2").arg(processName).arg(start ? "started" : "stopped");
        qDebug() << msg;
    } else {
        QString msg = QString("%1 service failed to %2").arg(processName).arg(start ? "start" : "stop");
        qDebug() << msg;
    }

    // Change the service to automatic if being started, or demand if stopped
    command = QString("sc config %1.exe start=%2").arg(processName).arg(start ? "auto" : "demand");
    if (exec.startDetached(command)) {
        qDebug() << "Successfully changed startup options for" << processName;
    } else {
        qDebug() << "Failed to change startup options for" << processName;
    }
    #endif
#endif
}

bool
ConsoleServer::restart()
{
    // If is running then stop the console server
    if (isRunning()) {
        setState(false);
    }

    // Start the console server
    setState(true);

    return isRunning();
}

bool
ConsoleServer::isConfigured()
{
    bool retval = true;

    m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    QString type = m_settings->value(WBSETTINGS_CONSOLESVR_TYPE,"http").toString();
    QString iface = m_settings->value(WBSETTINGS_CONSOLESVR_IF,"localhost").toString();
    int port = m_settings->value(WBSETTINGS_CONSOLESVR_PORT,0).toInt();
    QString sslKeyFile = m_settings->value(WBSETTINGS_CONSOLESVR_SSLKEY, "").toString();
    QString sslCertFile = m_settings->value(WBSETTINGS_CONSOLESVR_SSLCERT, "").toString();
    m_settings->endGroup();

    if (port <= 0) {
        retval = false;
    } else {
        if (type.toLower() == "https") {
            if (sslKeyFile.isEmpty() || sslCertFile.isEmpty())
                retval = false;
        }
    }

    return retval;
}

bool
ConsoleServer::setSSL(WickrIOSSLSettings *ssl)
{
    bool retval = false;

    m_settings->beginGroup(WBSETTINGS_CONSOLESVR_HEADER);
    if (m_settings->contains(WBSETTINGS_CONSOLESVR_TYPE)) {
        QString type = m_settings->value(WBSETTINGS_CONSOLESVR_TYPE,"http").toString();

        if (type.toLower() == "https") {
            m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLKEY, ssl->sslKeyFile);
            m_settings->setValue(WBSETTINGS_CONSOLESVR_SSLCERT, ssl->sslCertFile);
            m_settings->sync();
            retval = true;
        }
    }
    m_settings->endGroup();
    return retval;
}
