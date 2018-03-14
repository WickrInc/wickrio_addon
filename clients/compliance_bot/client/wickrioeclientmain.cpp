#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "wickrbotsettings.h"
#include "wickrioeclientmain.h"
#include "wickrIOJson.h"
#include "wickrbotprocessstate.h"
#include "wickrbotutils.h"
#include "wickrbotactiondatabase.h"

#include "user/wickrUser.h"
#include "libinterface/libwickrcore.h"
#include "messaging/wickrInbox.h"
#include "messaging/wickrSecureRoomMgr.h"
#include "common/wickrRuntime.h"

#include "wickrIOClientRuntime.h"
#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"
#include "complianceClientConfigInfo.h"
#include "wickrIOCommon.h"
#include "wickrIOBootstrap.h"


WickrIOEClientMain *WickrIOEClientMain::theBot;

/**
 * @brief WickrIOEClientMain::WickrIOEClientMain
 * This is the constructor for this class. Variables are initialized, the logins are created and
 * the m_logins list is set with those logins. Logging is started, counts initialized and SLOTS
 * are setup to receive specific SIGNALs
 */
WickrIOEClientMain::WickrIOEClientMain(OperationData *operation) :
    m_operation(operation),
    m_loginHdlr(operation, ClientVersionInfo::versionForLogin()),
    m_txIPC(this),
    m_rxIPC(0),
    m_timerStatsTicker(0),
    m_waitingForPassword(false)
{
    if( isVERSIONDEBUG() ) {
        m_operation->cleanUpSecs = 20;
        m_operation->startRcvSecs = 2;
    } else {
        m_operation->cleanUpSecs = 300;
        m_operation->startRcvSecs = 2;
    }

    m_rxThread = new WickrIOReceiveThread();

    this->connect(this, &WickrIOEClientMain::started, this, &WickrIOEClientMain::processStarted);
    this->connect(this, &WickrIOEClientMain::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

    this->connect(&m_loginHdlr, &WickrIOClientLoginHdlr::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);
    this->connect(&m_loginHdlr, &WickrIOClientLoginHdlr::signalLoginSuccess, this, &WickrIOEClientMain::slotLoginSuccess);

    WickrSwitchboardService *swbsvc = WickrCore::WickrRuntime::swbSvc();
    WickrMessageService *msgsvc = WickrCore::WickrRuntime::msgSvc();
    WickrTaskService *tasksvc = WickrCore::WickrRuntime::taskSvc();
    if (swbsvc && msgsvc && tasksvc) {
        // Signals from switchboard
        connect(swbsvc,
                &WickrSwitchboardService::signalState,
                this,
                &WickrIOEClientMain::slotSwitchboardServiceState);
#if 0
        connect(swbsvc,
                &WickrSwitchboardService::signalUserVideoUpdate,
                this,
                &WickrIOEClientMain::slotUserVideoUpdate);
#endif
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminUserSuspend,
                this,
                &WickrIOEClientMain::slotAdminUserSuspend);
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminDeviceSuspend,
                this,
                &WickrIOEClientMain::slotAdminDeviceSuspend);
        connect(swbsvc,
                &WickrSwitchboardService::signalDownloadAtLoginStart,
                this,
                &WickrIOEClientMain::slotMessageDownloadStatusStart);

#if 0
        connect(swbsvc,
                &WickrSwitchboardService::signalProfileImageUpdated,
                this,
                &WickrIOEClientMain::slotProfileImageUpdated);
#endif

        // Signals from message service
        connect(msgsvc,
                &WickrMessageService::signalState,
                this,
                &WickrIOEClientMain::slotMessageServiceState);
        connect(msgsvc,
                &WickrMessageService::signalSuspendedAccount,
                this,
                &WickrIOEClientMain::slotSetSuspendError);
        connect(msgsvc,
                &WickrMessageService::signalSuspendedAccount, this, [=] {
#if 0
            emit sigLogout();
#endif
            qDebug() << "Your Wickr ID has been suspended. If you feel this is in error please contact us by email at support@wickr.com" << "Suspended account";
        });
        connect(msgsvc,
                &WickrMessageService::signalDownloadAtLoginUpdate,
                this,
                &WickrIOEClientMain::slotMessageDownloadStatusUpdate);
        connect(msgsvc,
                &WickrMessageService::signalDownloadAtLoginEnd,
                this,
                &WickrIOEClientMain::slotOnLoginMsgSynchronizationComplete);

        // Signals from task service
        //
        connect(tasksvc,
                &WickrTaskService::signalState,
                this,
                &WickrIOEClientMain::slotTaskServiceState);

        qDebug() << "MESSAGE SERVICES: Application connections initialized.";

    } else {
        if (!swbsvc || !msgsvc || !tasksvc)
            qDebug() << "WickrIOEClientMain(): SWITCHBOARD, MESSAGE, TASK SERVICES - Not initialized.";
    }

    WickrCore::WickrSecureRoomMgr *roomMgr = WickrCore::WickrRuntime::getRoomMgr();
    if (roomMgr) {
        // Secure Room Convo Manager (SIGNALS)
#if 0
        connect(roomMgr,
                &WickrCore::WickrSecureRoomMgr::signalActivateRoom,
                convoActivator,
                &ActiveConvoController::slotActivateConvo);
        connect(roomMgr,
                &WickrCore::WickrSecureRoomMgr::signalActivateLandingPage,
                this,
                &wickrQuickMain::slotActivateLandingPage);
#endif
    }

    // Attached to the Watchdog service signal(s)
    WickrIOWatchdogService *wdSvc = WickrIOClientRuntime::wdSvc();
    if (wdSvc != nullptr) {
        connect(wdSvc, &WickrIOWatchdogService::signalServiceNotLoggedIn,
                this, &WickrIOEClientMain::slotServiceNotLoggedIn,
                Qt::QueuedConnection);
    } else {
        qDebug() << "WickrIOEClientMain(): Watchdog Service not initialized!";
    }
}


/**
 * @brief WickrIOEClientMain::~WickrIOEClientMain
 * This is the destructor for this class. The timer is stopped and the DB closed.
 */
WickrIOEClientMain::~WickrIOEClientMain()
{
    if (timer.isActive()) {
        timer.stop();
        timer.deleteLater();
    }

    if (m_operation->m_botDB != NULL) {
        m_operation->m_botDB->close();
        m_operation->m_botDB->deleteLater();
        m_operation->m_botDB = NULL;
    }

    if (m_operation->m_client != NULL) {
        delete m_operation->m_client;
        m_operation->m_client = NULL;
    }

    QCoreApplication::processEvents();
}

/**
 * @brief WickrIOEClientMain::slotLoginSuccess
 * This slot is called when the login is successful
 */
void WickrIOEClientMain::slotLoginSuccess()
{
    // Execute database load
    WickrDatabaseLoadContext *c = new WickrDatabaseLoadContext(WickrUtil::dbDump);
    connect(c, &WickrDatabaseLoadContext::signalRequestCompleted, this, &WickrIOEClientMain::slotDatabaseLoadDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOEClientMain::slotDatabaseLoadDone
 * Complete database load.
 */
void WickrIOEClientMain::slotDatabaseLoadDone(WickrDatabaseLoadContext *context)
{
    // Cleanup request
    context->deleteLater();

    // Need to display the Keys. Need to do here AFTER Database was loaded
    WickrCore::WickrUser *selfUser = WickrCore::WickrUser::getSelfUser();
    qDebug() << "********************************************************************";
    qDebug() << "**** USER SIGNING KEY";
    qDebug() << "**** You will need this to enter into the console for the Bot";
    qDebug() << "****";
    qDebug() << "" << QString(selfUser->getUserSigningKey().toHex());
    qDebug() << "********************************************************************";

    sendConsoleMsg(WBIO_IPCMSGS_USERSIGNKEY, QString(selfUser->getUserSigningKey().toHex()));

    emit signalLoginSuccess();

    // Tell the watchdog service that we are logged in
    WickrIOWatchdogService *wdSvc = WickrIOClientRuntime::wdSvc();
    if (wdSvc != nullptr) {
        wdSvc->setLoggedIn(true);
    }

    sendConsoleMsg(WBIO_IPCMSGS_STATE, "loggedin");

    // Update switchboard login credentials (login is performed only if not already logged in)
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin(),
                                         true);

    // Start the receive thread
    connect(m_rxThread, &WickrIOReceiveThread::signalProcessStarted, this, &WickrIOEClientMain::slotRxProcessStarted, Qt::QueuedConnection);
    m_rxThread->start();
}

void WickrIOEClientMain::slotRxProcessStarted()
{
    connect(m_rxThread, &WickrIOReceiveThread::signalReceivingStarted, this, &WickrIOEClientMain::slotRxProcessReceiving, Qt::QueuedConnection);

    //TODO: Change this to the clientruntime model
    QMetaObject::invokeMethod(m_rxThread, "slotStartReceiving", Qt::QueuedConnection);
}

void WickrIOEClientMain::slotRxProcessReceiving()
{
    // ONLINE: Login successful, so login to message service
    WickrCore::WickrRuntime::msgSvcLogin();

    startTimer();
#if 0
    // Emit update signals
    emit signalUserListUpdated();

    // Complete user setup as part of login
    processUserSetup();
#endif
}



/**
 * @brief slotAdminUserSuspend (SWITCHBOARD SIGNAL)
 * Administrator forced logout received via switchboard.
 */
void WickrIOEClientMain::slotAdminUserSuspend(SBSessionError code, const QString& reason)
{
    switch (code) {
    case SBSessionError::SBS_SSO_EXPIRED:
        break;
    default:
        // Display Informational Message
        qDebug() << "Your account has been suspended!\nREASON: " << reason;

        // TODO: Need to send a notification to an admin

    }
#if 0
    // Force logout
    slotOnLogout(false);
#endif
}

/**
 * @brief slotAdminDeviceSuspend (SWITCHBOARD SIGNAL)
 * Administrator forced logout and reset received via switchboard. Essentially a device reset/suspension.
 */
void WickrIOEClientMain::slotAdminDeviceSuspend()
{
    // Display Informational Message
    qDebug() << "System has reset this device as a result of either a forgot password performed by user, or a user account deletion.";

#if 0
    // Logout and Reset device
    slotResetDevice();
#endif
}

void WickrIOEClientMain::slotSetSuspendError() {
#if 0
    m_gotSuspendError = true;
    logout();
    m_gotSuspendError = false;
#endif
}

/**
 * @brief slotMessageDownloadStatusStart
 * Slot called to begin tracking progress of message download.
 * NOTE: Used to display progress on initial login only.
 * @param msgsToDownload
 */
void WickrIOEClientMain::slotMessageDownloadStatusStart(int msgsToDownload)
{
    qDebug() << "Messages to download:" << msgsToDownload;
}

void WickrIOEClientMain::slotMessageDownloadStatusUpdate(int msgsDownloaded)
{
    Q_UNUSED(msgsDownloaded);
}



/**
 * @brief WickrIOEClientMain::pauseAndExitSlot
 * Call this slot to put the state of the client into the DOWN state,
 * and exit the client application.
 */
void WickrIOEClientMain::stopAndExitSlot()
{
    sendConsoleMsg(WBIO_IPCMSGS_STATE, "stopping");

    stopAndExit(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOEClientMain::pauseAndExitSlot
 * Call this slot to put the state of the client into the paused state,
 * and exit the client application.
 */
void WickrIOEClientMain::pauseAndExitSlot()
{
    stopAndExit(PROCSTATE_PAUSED);
}

/**
 * @brief slotGotMessage
 * @param type
 * @param value
 */
void WickrIOEClientMain::slotReceivedMessage(QString type, QString value)
{
    if (type.toLower() == WBIO_IPCMSGS_PASSWORD) {
        // Check if we are waiting for the password to proceed with the login
        qDebug() << "Proceeding with the login";
        if (m_waitingForPassword) {
            m_waitingForPassword = false;

            QMap<QString,QString> qmapval = WickrIOIPCCommands::parsePasswordValue(value);
            if (qmapval.size() > 0) {
                m_password = qmapval.value(WBIO_BOTINFO_PASSWORD);
                loadBootstrapFile();
                m_loginHdlr.addLogin(m_user, m_password, m_userName, "");
                m_loginHdlr.initiateLogin();
            }
        }
    }
}

void WickrIOEClientMain::processStarted()
{
    qDebug() << "Started WickrIOEClientMain";

    if (! startTheClient())
        stopAndExit(PROCSTATE_DOWN);
}

bool
WickrIOEClientMain::sendConsoleMsg(const QString& cmd, const QString& value)
{
    QString message = QString("%1=%2").arg(cmd).arg(value);

    WickrBotProcessState state;
    if (!m_operation->m_botDB->getProcessState(WBIO_PROVISION_TARGET, &state))
        return false;

    if (! m_txIPC.sendMessage(state.ipc_port, message)) {
        return false;
    }

    QTimer timer;
    QEventLoop loop;

    loop.connect(&m_txIPC, SIGNAL(signalSentMessage()), SLOT(quit()));
    loop.connect(&m_txIPC, SIGNAL(signalSendError()), SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    int loopCount = 6;

    while (loopCount-- > 0) {
        timer.start(10000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            break;
        } else {
            qDebug() << "Timed out waiting for stop client message to send!";
        }
    }
    return true;
}

/**
 * @brief WickrIOEClientMain::setIPC
 * Save the IPC object.  Only really need to make a connection to receive the stop
 * signal when the applicaiton is to be closed.
 * @param ipc
 */
void WickrIOEClientMain::setIPC(WickrIOIPCService *ipc)
{
    m_rxIPC = ipc;
    connect(ipc, &WickrIOIPCService::signalGotStopRequest, this, &WickrIOEClientMain::stopAndExitSlot);
    connect(ipc, &WickrIOIPCService::signalGotPauseRequest, this, &WickrIOEClientMain::pauseAndExitSlot);
    connect(ipc, &WickrIOIPCService::signalReceivedMessage, this, &WickrIOEClientMain::slotReceivedMessage);
}

void WickrIOEClientMain::slotDoTimerWork()
{
    if (m_rxIPC != NULL && m_rxIPC->isRunning()) {
        m_rxIPC->check();
    }

    m_timerStatsTicker++;
}

/**
 * @brief WickrIOClientRuntime::slotServiceNotLoggedIn
 * This slot is called when a thread has been found to be unlogged
 * in for longer than a good period, so we will stop the client to
 * refresh things.
 */
void
WickrIOEClientMain::slotServiceNotLoggedIn()
{
    sendConsoleMsg(WBIO_IPCMSGS_STATE, "stopping");

    stopAndExit(PROCSTATE_DOWN);
}



/**
 * @brief WickrIOEClientMain::stopAndExit
 * This is called to exit the application. The application state is set to the input
 * state in the process_state table in the database.
 */
void WickrIOEClientMain::stopAndExit(int procState)
{
    // Set the process state for after the shutdown
    WickrIOClientRuntime::wdSetShutdownState(procState);

    // logout of switchboard service
    WickrCore::WickrRuntime::swbSvcLogout();

    // logout
//    WickrCore::WickrRuntime::msgSvcLogout();

    // logout to task service (NOTE: only if closing application)
    WickrCore::WickrRuntime::taskSvcLogout();

//    WickrService::requestLogoff();

    // TODO: Change to the clientruntime model
    QMetaObject::invokeMethod(m_rxThread, "slotStopReceiving", Qt::QueuedConnection);

    if (m_rxThread->m_threadState != Idle) {
        // TODO: Need to make sure the callbackservice is done

        // Wait till the action/cleanup/rcvmsg processes are finished
        QTimer timer;
        QEventLoop loop;

        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

        while (m_rxThread->m_threadState != Idle ) {
            //TODO: need to make sure the callback service is stopped
            if (m_rxThread->m_threadState != Idle) {
                qDebug() << "Waiting for client Receive Messages process to finish!";
                QMetaObject::invokeMethod(m_rxThread, "stopProcessing", Qt::QueuedConnection);
            }
#if 0
            if ( m_callbackThread->m_threadState != Idle) {
                qDebug() << "Waiting for Callback Thread process to finish!";
                QMetaObject::invokeMethod(m_callbackThread, "stopProcessing", Qt::QueuedConnection);
            }
#endif

            timer.start(1000);
            loop.exec();
        }
    }

    // If the logins have failed, make sure the DB is open so the state can be changed
    if (m_loginHdlr.getLoginState() == LoginsFailed) {
        if (m_operation->m_botDB == NULL)
            m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);
    }

    if (m_operation->m_botDB != NULL) {
        if (! m_operation->m_botDB->isOpen()) {
            m_operation->m_botDB->deleteLater();
            m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);
        }
    }

    if (m_loginHdlr.getLoginState() == LoggedIn) {
        if (WickrCore::WickrSession::getActiveSession()) {
            WickrCore::WickrSession::getActiveSession()->closeSession(true);
        }
        WickrDBAdapter::closeDB();
    }

    QCoreApplication::quit();
//    QCoreApplication::exit(1);
}

/**
 * @brief WickrIOEClientMain::startTheClient
 * This method will start the client running, if it is not active already. The process
 * of starting the client begins with the login process.
 * @return  True is returned if the process is initiated. False if already active.
 */
bool WickrIOEClientMain::startTheClient()
{
    // Check if there is already a WickrBotClient running
    if (m_operation->alreadyActive()) {
        return false;
    }

    m_operation->log_handler->log("Starting");

    // Open the database if needed and get the Client information
    if (m_operation->m_botDB == NULL) {
        m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);
    }

    // TASK SERVICE: Utility service login here (to ensure rest api available)
    WickrCore::WickrRuntime::taskSvcLogin();

    if (!m_waitingForPassword) {
        m_loginHdlr.initiateLogin();
    }

    emit signalStarted();
    return true;
}

/**
 * @brief WickrIOEClientMain::slotSwitchboardState (SWITCHBOARD SIGNAL)
 * Slot accepts state change update signals from switchboard service
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotSwitchboardServiceState(WickrServiceState state, SBSessionStatus sessionStatus, const QString& text)
{
    switch(state) {
    case WickrServiceState::SERVICE_LOGGED_IN:
        // Finish login
#if 0 // TODO: implement this, change the login process
        completePostLogin();
#endif
        break;

    case WickrServiceState::SERVICE_LOGGED_OUT:
#if 0
        switch(sessionStatus) {
        case SBSessionStatus::NETWORK_ERROR:
            // NETWORK ERROR STATE: Update network status, and reset interfaces
            //
            // Network status
            m_quickMain->getDashboardData()->slotNetworkState(false);

            // Flag Task service to reset communications (direct call, although could be called by signal)
            WickrCore::WickrRuntime::taskSvcResetNetwork();

            // Flag Message service to reset communications (signals)
            WickrCore::WickrRuntime::msgSvcResetNetwork();
            break;

        case SBSessionStatus::LOGIN_AUTH_FAIL:
            // LOGIN FAILURE: Login failure is due to invalid authorization token, so let's perform relogin.
            //
            slotReloginStart();
            break;

        case SBSessionStatus::LOGIN_TIMEOUT:
            // LOGIN TIMEOUT: Timeout waiting for login response.
            //
            showError("Switchboard login has timed out, you are being logged out.");
            WickrCore::WickrRuntime::swbSvcLogout();
            logout();
            break;

        case SBSessionStatus::SENDER_VALIDATION_ERROR:
            // SENDER VALIDATION ERROR: Sender validation error occurred, this forces logoff to ensure in-order
            //                          processing without undue code complexity. Should only be in cases of network failure.
            //
            showError("Server connectivity error has occurred, you are being logged out. No data loss.");
            WickrCore::WickrRuntime::swbSvcLogout();    // Force switchboard logout immediately
            logout();
            break;

        default:
            // CHECK CLIENT STATE: If switchboard service logs out and client is in LOGIN_COMPLETE state, then
            //                     force logout of client, as it is most likely an availability issue.
            if (m_clientState == LOGIN_COMPLETE) {
                showError("Switchboard currently unavailable, you are being logged out.");
                WickrCore::WickrRuntime::swbSvcLogout();
                logout();
            }
            break;
        }
#endif

    default:
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotMessageServiceState
 * @param state
 * @param text
 */
void WickrIOEClientMain::slotMessageServiceState(WickrServiceState state)
{
    switch(state) {
    case WickrServiceState::SERVICE_LOGGED_IN:
        // Register service with active session.
        WickrCore::WickrSession::serviceLoggedIn("MessageService");

#if 0
        // ROOM MANAGEMENT: recover or check recovery status
        if (m_personalize.isFirstLogin()) {
            // Recover rooms (on first login)
            recoverRooms();
        } else {
            // Report unrecovered rooms status
            recoverStatus();
        }
        m_personalize.setFirstLogin(false);
#endif

        // MESSAGE CHECK: Perform initial message check
        WickrCore::WickrRuntime::msgSvcScheduleCheck(ON_LOGIN);
        break;

    case WickrServiceState::SERVICE_LOGGED_OUT:
        // Unregister service with active session.
        // This will block closing the session and database until all services have unregistered.
        WickrCore::WickrSession::serviceLoggedOut("MessageService");
        break;

    default:
        break;
    }
}


/**
 * @brief WickrIOEClientMain::slotTaskServiceState
 * Slot accepts state change update signals from task service.
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotTaskServiceState(WickrServiceState state)
{
    switch(state) {
    case WickrServiceState::SERVICE_LOGGED_IN:
        WickrCore::WickrSession::serviceLoggedIn("TaskService");
        break;
    case WickrServiceState::SERVICE_LOGGED_OUT:
        WickrCore::WickrSession::serviceLoggedOut("TaskService");
        break;
    default:
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotOnLoginMsgSynchronizationComplete()
 * This slot is called whenever the initial download is complete after
 * a switchboard login (or login after recovery). If there is any further
 * UI "bootstrap" processing, it will be continued here.
 *
 * ONLINE LOGIN: This slot will be triggered after switchboard login complete, and
 *               first initial download complete.
 *
 * OFFLINE LOGIN: Since we are offline, switchboard login and download are not executed,
 *                so this slot is called directly from completeLogin().
 */
void WickrIOEClientMain::slotOnLoginMsgSynchronizationComplete()
{
    // ONLINE PROCESSING

    // Perform immediate houskeeping after download (backups if required, unsuspend convo backup after login)
    WickrCore::WickrRuntime::taskSvc()->suspendConvoBackUp(false);
    WickrConvoBackupContext *c = new WickrConvoBackupContext();
    WickrCore::WickrRuntime::taskSvcMakeRequest(c, true);

    // Start housekeeping timer (i.e. periodic backup check on interval)
//    housekeepingTimerStart();

    // After initial download complete, login to switchboard
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin(),
                                         false);
}

bool WickrIOEClientMain::parseSettings(QSettings *settings)
{
    /*
     * Parse out the settings associated with the User
     */
    settings->beginGroup(WBSETTINGS_USER_HEADER);
    QString user = settings->value(WBSETTINGS_USER_USER, "").toString();
    QString password = settings->value(WBSETTINGS_USER_PASSWORD, "").toString();
    QString username = settings->value(WBSETTINGS_USER_USERNAME, "").toString();
    QString transactionID = settings->value(WBSETTINGS_USER_TRANSACTIONID, "").toString();
    settings->endGroup();

    if (user.isEmpty()) {
        qDebug() << "User is not set";
        return false;
    }

    if (password.isEmpty()) {
        m_waitingForPassword = true;
    } else {
        loadBootstrapFile();
        m_loginHdlr.addLogin(user, password, username, transactionID);
        m_waitingForPassword = false;
    }

    // Save for use in main
    m_user = user;
    m_password = password;
    m_userName = username;

    return true;
}

bool
WickrIOEClientMain::loadBootstrapFile()
{
    QString clientDbDir = WickrAppContext::getFilesDir();

    //  Read in the bootstrap file
    QString bootstrapFilename = clientDbDir + "bootstrap";
    if (QFile::exists(bootstrapFilename)) {
        QString bootstrapString = WickrIOBootstrap::readFile(bootstrapFilename, m_password);
        if (bootstrapString == nullptr) {
            qDebug() << "Cannot read the bootstrap file!";
            return false;
        }
        return loadBootstrapString(bootstrapString);
    }
    return false;
}

/**
 * @brief WickrIOEClientMain::loadBootstrapString
 * Take the decrypted bootstrap string and call the environment manager to apply
 * @param bootstrapStr
 * @return
 */
bool
WickrIOEClientMain::loadBootstrapString(const QString& bootstrapStr)
{
    QJsonDocument d;
    d = d.fromJson(bootstrapStr.toUtf8());
    if (!WickrCore::WickrRuntime::getEnvironmentMgr()->loadBootStrapJson(d))
    {
        qDebug() << "Incorrect credentials - please try again. Configuration file error";
    }
    return false;
}


