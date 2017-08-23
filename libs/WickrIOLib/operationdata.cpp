#include <QCoreApplication>
#include <QDateTime>

#include "operationdata.h"
#include "wickrbotdatabase.h"
#include "wickrbotutils.h"

OperationData::OperationData() :
    debug(false),
    force(false),
    messageCount(0),
    m_botDB(NULL),
    m_client(NULL),
    receiveOn(false),
    m_appTimeOut(180),
    m_pid(QCoreApplication::applicationPid()),
    m_wbLog(NULL),
    m_handleInbox(false)
{
    m_waiting4image = false;

    outputFile = "";
    gotOutputFile = false;

    inputFile = "";
    gotInputFile = false;

    attachmentsDir = "";
    databaseDir = "";
    messagesDir = "";

    // Create a file download object to handle downloading attachments
#ifndef DO_WICKRBOT
    m_pImgCtrl = new FileDownloader(this);
    connect(m_pImgCtrl, SIGNAL(downloaded()), this, SLOT(loadImage()));
    connect(m_pImgCtrl, SIGNAL(downloadFailed()), this, SLOT(loadFailed()));
#endif
}

OperationData::~OperationData()
{
    m_pImgCtrl->deleteLater();
    if (m_client != NULL) {
        delete m_client;
        m_client = NULL;
    }

    if (m_wbLog != NULL) {
        delete m_wbLog;
        m_wbLog = NULL;
    }

    if (m_botDB != NULL) {
        if (m_botDB->isOpen()) {
            m_botDB->close();
            m_botDB->deleteLater();
            m_botDB = NULL;
        }
    }

    QCoreApplication::processEvents();
}

bool OperationData::busy()
{
    return true;
#if 0
    if (m_waiting4image)
        return true;
    return false;
#endif
}

#ifndef DO_WICKRBOT
bool OperationData::downloadImage(QUrl imageURL, QString fullpath)
{
    m_waiting4image = true;
    m_imageSuccess = false;
    m_imageFileName = fullpath;
    m_pImgCtrl->getFile(imageURL, true);
    return m_imageSuccess;
}

void OperationData::loadImage()
{
    QFile *file;

    if (m_imageFileName.length() > 0) {
        file = new QFile;
        file->setFileName(m_imageFileName);
        file->open(QIODevice::WriteOnly);
        file->write(m_pImgCtrl->downloadedData());
        file->close();
        file->deleteLater();
        m_imageSuccess = true;
    } else {
        m_imageSuccess = false;
    }
    m_waiting4image = false;
}

void OperationData::loadFailed()
{
    m_waiting4image = false;
}
#endif

void OperationData::setupLog(const QString &logFileName)
{
    m_wbLog = new WickrBotLog(logFileName);
}

QDateTime OperationData::lastLogTime()
{
    return m_wbLog->lastLogTime();
}

QString OperationData::getLogFile()
{
    if (m_wbLog == NULL)
        return QString("");
    return m_wbLog->getFileName();
}

bool OperationData::setApiKey(const QString &apiKey)
{
    m_client = m_botDB->getClientUsingApiKey(apiKey);
    return m_client != NULL;
}

bool OperationData::validateApiKey(const QString &apiKey)
{
    if (m_client != NULL) {
        return m_client->apiKey == apiKey;
    }
    return false;
}

QString OperationData::getResponseURL()
{
    QString sendMsgURL;

    if (m_client) {
        sendMsgURL = QString("%1://%2/%3/Apps/%4/Messages")
                        .arg(m_client->isHttps ? "https" : "http")
                        .arg(m_client->iface)
                        .arg(m_client->port)
                        .arg(m_client->apiKey);
    }
    return sendMsgURL;
}


/**
 * @brief OperationData::alreadyActive
 * This method will check if this application is currently running. This is necessary just
 * in case the application is hung.
 * TODO: Need to add code to kill a hung application.
 * @return  True is returned if the application is already running.
 */
bool OperationData::alreadyActive(bool closeDbOnExit)
{
    QDateTime lastLogTime = this->lastLogTime();
    qDebug() << "LastLogTime=" << lastLogTime.toString(LOGS_DATETIME_FORMAT);

    WickrBotProcessState procState;
    if (m_botDB == NULL) {
        m_botDB = new WickrBotDatabase(databaseDir);
    }

    if (m_botDB->getProcessState(processName, &procState)) {
        /*
         * If the previous run is still in the running state then evaluate whether
         * that process is still running or not
         */
        if (procState.state == PROCSTATE_RUNNING) {
            qDebug() << "Process is still running, date=" << procState.last_update.toString(DB_DATETIME_FORMAT);

            const QDateTime dt = QDateTime::currentDateTime();
            int secs = procState.last_update.secsTo(dt);
            qDebug() << "Seconds since last status:" << QString::number(secs);


            // If less than 3 minutes then return failed, since the process is still running
            if (secs < m_appTimeOut) {
                m_botDB->close();
                m_botDB->deleteLater();
                m_botDB = NULL;

                // if forced return false, else return true
                return !force;
            }

            // Else it has been longer than 10 minutes, kill the old process and continue
            QString appName = QFileInfo(QCoreApplication::arguments().at(0)).fileName();
            if (WickrBotUtils::isRunning(appName, procState.process_id)) {
                log(QString("Killing old process, id=%1").arg(procState.process_id));
                WickrBotUtils::killProcess(procState.process_id);
            }
        }
    }

    m_botDB->updateProcessState(processName, m_pid, PROCSTATE_RUNNING);
    if (closeDbOnExit) {
        m_botDB->close();
        m_botDB->deleteLater();
        m_botDB = NULL;
    }
    return false;
}

/**
 * @brief OperationData::updateProcessState
 * This function will update the process state to the input state for this process.
 * The process ID will be checked first to make sure this process is the one in control
 * of the record in the database, unless the force flag is set to true.
 * @param state The new state to set
 * @param force If true the process_state will be set regardless of the PID in the DB
 * @return false if a problem occurs, or the PID is not set on the record in the DB
 */
bool
OperationData::updateProcessState(int state, bool force)
{
    if (m_botDB) {
        // If not forcing the set then check if the PID matches
        if (!force) {
            WickrBotProcessState state;
            if (m_botDB->getProcessState(processName, &state)) {
                if (state.process_id != m_pid) {
                    return false;
                }
            }
        }
        return m_botDB->updateProcessState(processName, m_pid, state);
    }
    return false;
}
