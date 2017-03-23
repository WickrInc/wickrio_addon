#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QString>
#include <QNetworkInterface>

#include "wickrioconsoleclienthandler.h"
#include "wbio_common.h"
#include "server_common.h"
#include "wickrbotsettings.h"
#include "wickrbotprocessstate.h"
#include "wickrbotutils.h"

/**
 * @brief Client::addClient
 * This function will add a new client to the database. The input WickrIOClients class
 * contains all of the necessary information to add the new client.  All of the necessary
 * database and settings files will be created. The new client will be created in the
 * paused state, so it should not be started until explicitly started.
 * @param newClient pointer to the WickrIOClients class for the new client
 * @return an error string is returned if an error occurs during processing. If no error then
 * the returned string is empty
 */
QString
WickrIOConsoleClientHandler::addClient(WickrIOClientDatabase *ioDB, WickrIOClients *newClient)
{
    // If the db is open the proceed, otherwise return false
    if (! ioDB->isOpen()) {
        return QCoreApplication::tr("Database is not open!");
    }

    // Create the configuration file
    QString dbDir(ioDB->m_dbDir);
    QString logname;
    QString configFileName;
    QString clientDbDir;
    QString attachDir;

#ifdef Q_OS_WIN
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT)
            .arg(WBIO_ORGANIZATION)
            .arg(WBIO_GENERAL_TARGET)
            .arg(newClient->name);
#else
    configFileName = QString(WBIO_CLIENT_SETTINGS_FORMAT).arg(ioDB->m_dbDir).arg(newClient->name);
#endif
    clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(ioDB->m_dbDir).arg(newClient->name);
    logname = QString(WBIO_CLIENT_LOGFILE_FORMAT).arg(ioDB->m_dbDir).arg(newClient->name);
    attachDir = QString(WBIO_CLIENT_ATTACHDIR_FORMAT).arg(ioDB->m_dbDir).arg(newClient->name);

    qDebug() << "**********************************";
    qDebug() << "startClient: command line arguments for" << newClient->name;
    qDebug() << "dbDir=" << dbDir;
    qDebug() << "logname=" << logname;
    qDebug() << "Config File=" << configFileName;
    qDebug() << "Client DB Dir=" << clientDbDir;
    qDebug() << "Attachments dir=" << attachDir;
    qDebug() << "**********************************";

    // The client DB directory MUST exist already
    if (! QDir(ioDB->m_dbDir).exists()) {
        return QString("%1 does not exist").arg(ioDB->m_dbDir);
    }

    QString clientDir;

    clientDir = QString("%1/clients").arg(ioDB->m_dbDir);
    if (! QDir(clientDir).exists()) {
        if (! QDir().mkdir(clientDir)) {
            return QCoreApplication::tr("Cannot create the %1 directory").arg(clientDir);
        }
    }

    clientDir = QString("%1/clients/%2").arg(ioDB->m_dbDir).arg(newClient->name);
    if (! QDir(clientDir).exists()) {
        qDebug() << clientDir << "does not exist";
        if (! QDir().mkdir(clientDir)) {
            return QCoreApplication::tr("Cannot create the %1 directory").arg(clientDir);
        }
    }

    QFile file(configFileName);

    if (file.exists()) {
        file.remove();
    }
    QSettings * settings = new QSettings(configFileName, QSettings::NativeFormat);

    settings->beginGroup(WBSETTINGS_USER_HEADER);
    settings->setValue(WBSETTINGS_USER_USER, newClient->user);
    settings->setValue(WBSETTINGS_USER_PASSWORD, newClient->password);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_DATABASE_HEADER);
    settings->setValue(WBSETTINGS_DATABASE_DIRNAME, dbDir);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_LOGGING_HEADER);
    settings->setValue(WBSETTINGS_LOGGING_FILENAME, logname);
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_LISTENER_HEADER);
    settings->setValue(WBSETTINGS_LISTENER_PORT, newClient->port);
    // If using localhost interface then do not need the host entry in the settings
    if (newClient->iface == "localhost") {
        settings->remove(WBSETTINGS_LISTENER_IF);
    } else {
        settings->setValue(WBSETTINGS_LISTENER_IF, newClient->iface);
    }
    // If is HTTPS then save the SSL settings, otherwise remove them
    if (newClient->isHttps) {
        settings->setValue(WBSETTINGS_LISTENER_SSLKEY, newClient->sslKeyFile);
        settings->setValue(WBSETTINGS_LISTENER_SSLCERT, newClient->sslCertFile);
    } else {
        settings->remove(WBSETTINGS_LISTENER_SSLKEY);
        settings->remove(WBSETTINGS_LISTENER_SSLCERT);
    }
    settings->endGroup();

    settings->beginGroup(WBSETTINGS_ATTACH_HEADER);
    settings->setValue(WBSETTINGS_ATTACH_DIRNAME, attachDir);
    settings->endGroup();

    settings->sync();

    // Insert a client record into the database
    if (newClient->id > 0) {
        if (!ioDB->updateClientsRecord(newClient)) {
            return QCoreApplication::tr("Could not update clients record!");
        }
    } else {
        if (!ioDB->insertClientsRecord(newClient)) {
            return QCoreApplication::tr("Could not create clients record!");
        }
    }

    QString processName = WBIOCommon::getClientProcessName(newClient->name);

    // Set the state of the client process to paused
    if (! ioDB->updateProcessState(processName, 0, PROCSTATE_PAUSED)) {
        return QCoreApplication::tr("Could not create process state record!");
    }
    return QString("");
}

/**
 * @brief WickrIOConsoleClientHandler::getNetworkInterfaceList
 * This function will return a list of the network interfaces that can be
 * used to accept WickrIO network traffic. The first entry will always be the
 * "localhost" entry.
 * @return
 */
QStringList WickrIOConsoleClientHandler::getNetworkInterfaceList()
{
    QStringList interfaceList;
    interfaceList.append(QString("localhost"));

#if 0
    foreach (QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            interfaceList.append(iface.name());
        }
    }
#else
    QNetworkInterface interface;
    QList<QHostAddress> IpList = interface.allAddresses();

    foreach (QHostAddress addr, IpList) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress(QHostAddress::LocalHost)) {
            interfaceList.append(addr.toString());
        }
    }
#endif
    return interfaceList;
}

bool
WickrIOConsoleClientHandler::validateSSLKey(const QString &sslKeyFile)
{
    QFile sslFile(sslKeyFile);
    QFileInfo info(sslFile);

    if (sslFile.exists() && info.isFile()  && info.size() > 0) {
        return true;
    }
    return false;
}

bool
WickrIOConsoleClientHandler::validateSSLCert(const QString &sslCertFile)
{
    QFile sslFile(sslCertFile);
    QFileInfo info(sslCertFile);

    if (sslFile.exists() && info.isFile() && info.size() > 0) {
        return true;
    }
    return false;
}

/**
 * @brief WickrIOConsoleClientHandler::getActualProcessState
 * This function will get the process state from the WickrIO database. If the state is NOT
 * down or paused it will also verify that the process is running.
 * TODO: Should the process be updated in the database if it is found to not be running?
 * @param processName
 * @param ioDB
 * @return
 */
QString
WickrIOConsoleClientHandler::getActualProcessState(const QString &processName, WickrIOClientDatabase* ioDB, int timeout)
{
    WickrBotProcessState state;
    QString clientState = "UNKNOWN";

    if (ioDB->getProcessState(processName, &state)) {
        if (state.state == PROCSTATE_DOWN) {
            clientState = "Down";
        } else if (state.state == PROCSTATE_RUNNING) {
            clientState = "Running";

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
                    clientState = "Hung?";
                } else {
                    clientState = "Not Running";
                }
            }

        } else if (state.state == PROCSTATE_PAUSED) {
            clientState = "Paused";
        }
    }

    return clientState;
}
