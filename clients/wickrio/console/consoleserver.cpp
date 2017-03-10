#include <QProcess>
#include <QDebug>
#include <QString>
#include <QCoreApplication>

#include "consoleserver.h"
#include "wickrbotprocessstate.h"
#include "wbio_common.h"
#include "wickrbotutils.h"
#include "wickrbotsettings.h"

ConsoleServer::ConsoleServer(WickrIOClientDatabase *ioDB) :
    m_ioDB(ioDB)
{
    m_settings = WBIOCommon::getSettings();
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

void
ConsoleServer::toggleState(const QString &processName)
{
    QCoreApplication::processEvents();
    bool start;
    if (isRunning(processName)) {
        // Command to stop the Clients Server Daemon/Service
        start = false;
    } else {
        start = true;
    }

#ifdef Q_OS_LINUX
        QProcess exec;
        QString command = QString("/etc/init.d/%1 %2").arg(processName).arg(start ? "start" : "stop");

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
        toggleState();
    }

    // Start the console server
    toggleState();

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
