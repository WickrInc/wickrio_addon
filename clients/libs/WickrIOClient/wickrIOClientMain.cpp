#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "wickrbotsettings.h"
#include "wickrIOClientMain.h"
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
#include "wickrIOCommon.h"

WickrIOClientMain *WickrIOClientMain::theBot;

/**
 * @brief WickrIOClientMain::WickrIOClientMain
 * This is the constructor for this class. Variables are initialized, the logins are created and
 * the m_logins list is set with those logins. Logging is started, counts initialized and SLOTS
 * are setup to receive specific SIGNALs
 */
WickrIOClientMain::WickrIOClientMain(OperationData *operation, WickrIORxDetails *rxDetails, unsigned services) :
    m_operation(operation),
    m_services(services),
    m_loginHdlr(operation, ClientVersionInfo::versionForLogin()),
    m_rxDetails(rxDetails)
{
    if( isVERSIONDEBUG() ) {
        m_operation->cleanUpSecs = 20;
        m_operation->startRcvSecs = 2;
    } else {
        m_operation->cleanUpSecs = 300;
        m_operation->startRcvSecs = 2;
    }

    this->connect(this, &WickrIOClientMain::started, this, &WickrIOClientMain::processStarted);
    this->connect(this, &WickrIOClientMain::signalExit, this, &WickrIOClientMain::stopAndExitSlot);

    this->connect(&m_loginHdlr, &WickrIOClientLoginHdlr::signalExit, this, &WickrIOClientMain::stopAndExitSlot);
    this->connect(&m_loginHdlr, &WickrIOClientLoginHdlr::signalLoginSuccess, this, &WickrIOClientMain::slotLoginSuccess);



    WickrSwitchboardService *swbsvc = WickrCore::WickrRuntime::swbSvc();
    WickrMessageService *msgsvc = WickrCore::WickrRuntime::msgSvc();
    WickrTaskService *tasksvc = WickrCore::WickrRuntime::taskSvc();
    if (swbsvc && msgsvc && tasksvc) {
        // Signals from switchboard
        connect(swbsvc,
                &WickrSwitchboardService::signalState,
                this,
                &WickrIOClientMain::slotSwitchboardServiceState);
#if 0
        connect(swbsvc,
                &WickrSwitchboardService::signalUserVideoUpdate,
                this,
                &WickrIOClientMain::slotUserVideoUpdate);
#endif
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminUserSuspend,
                this,
                &WickrIOClientMain::slotAdminUserSuspend);
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminDeviceSuspend,
                this,
                &WickrIOClientMain::slotAdminDeviceSuspend);
#if 0
        connect(swbsvc,
                &WickrSwitchboardService::signalProfileImageUpdated,
                this,
                &WickrIOClientMain::slotProfileImageUpdated);
#endif
        connect(swbsvc,
                &WickrSwitchboardService::signalDownloadAtLoginStart,
                this,
                &WickrIOClientMain::slotMessageDownloadStatusStart);

        // Signals from message service
        connect(msgsvc,
                &WickrMessageService::signalState,
                this,
                &WickrIOClientMain::slotMessageServiceState);
        connect(msgsvc,
                &WickrMessageService::signalSuspendedAccount,
                this,
                &WickrIOClientMain::slotSetSuspendError);
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
                &WickrIOClientMain::slotMessageDownloadStatusUpdate);
        connect(msgsvc,
                &WickrMessageService::signalDownloadAtLoginEnd,
                this,
                &WickrIOClientMain::slotOnLoginMsgSynchronizationComplete);

        // Signals from task service
        //
        connect(tasksvc,
                &WickrTaskService::signalState,
                this,
                &WickrIOClientMain::slotTaskServiceState);

        qDebug() << "MESSAGE SERVICES: Application connections initialized.";

    } else {
        if (!swbsvc || !msgsvc || !tasksvc)
            qDebug() << "WickrIOClientMain(): SWITCHBOARD, MESSAGE, TASK SERVICES - Not initialized.";
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

    // Startup the action handler
    if (m_services & WICKRBOT_SERVICE_ACTIONSVC)
        m_actionService = new WickrIOActionService(operation);

    // Startup the event handler
    if (m_services & WICKRBOT_SERVICE_EVENTSVC)
        m_eventService = new WickrIOEventService(operation);

    // Attached to the Watchdog service signal(s)
    WickrIOWatchdogService *wdSvc = WickrIOClientRuntime::wdSvc();
    if (wdSvc != nullptr) {
        connect(wdSvc, &WickrIOWatchdogService::signalServiceNotLoggedIn,
                this, &WickrIOClientMain::slotServiceNotLoggedIn,
                Qt::QueuedConnection);
    } else {
        qDebug() << "WickrIOEClientMain(): Watchdog Service not initialized!";
    }
}


/**
 * @brief WickrIOClientMain::~WickrIOClientMain
 * This is the destructor for this class. The timer is stopped and the DB closed.
 */
WickrIOClientMain::~WickrIOClientMain()
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
 * @brief WickrIOClientMain::slotLoginSuccess
 * This slot is called when the login is successful
 */
void WickrIOClientMain::slotLoginSuccess()
{
    m_operation->postEvent("Logged in", false);

//    sendConsoleMsg(WBIO_IPCMSGS_USERSIGNKEY, userSigningKey);

    // Execute database load
    WickrDatabaseLoadContext *c = new WickrDatabaseLoadContext(WickrUtil::dbDump);
    connect(c, &WickrDatabaseLoadContext::signalRequestCompleted, this, &WickrIOClientMain::slotDatabaseLoadDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOClientMain::slotDatabaseLoadDone
 * Complete database load.
 */
void WickrIOClientMain::slotDatabaseLoadDone(WickrDatabaseLoadContext *context)
{
    // Cleanup request
    context->deleteLater();

    emit signalLoginSuccess();

    // Tell the watchdog service that we are logged in
    WickrIOWatchdogService *wdSvc = WickrIOClientRuntime::wdSvc();
    if (wdSvc != nullptr) {
        wdSvc->setLoggedIn(true);
    }

    // Update switchboard login credentials (login is performed only if not already logged in)
    WickrCore::WickrRuntime::swbSvcLogin(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                         WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                         WickrCore::WickrSession::getActiveSession()->getAppID(),
                                         WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                         WickrCore::WickrSession::getActiveSession()->getNetworkIdFromLogin(),
                                         true);

    // Start the receive service
    m_rxService = new WickrIORxService(m_operation, m_rxDetails);
    connect(m_rxService, &WickrIORxService::signalProcessStarted, this, &WickrIOClientMain::slotRxProcessStarted, Qt::QueuedConnection);
}

void WickrIOClientMain::slotRxProcessStarted()
{
    connect(m_rxService, &WickrIORxService::signalReceivingStarted, this, &WickrIOClientMain::slotRxProcessReceiving, Qt::QueuedConnection);
    m_rxService->startReceive();
}

void WickrIOClientMain::slotRxProcessReceiving()
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
void WickrIOClientMain::slotAdminUserSuspend(SBSessionError code, const QString& reason)
{
    m_operation->postEvent("User is suspended!", true);

    switch (code) {
    case SBSessionError::SBS_SSO_EXPIRED:
        break;
    default:
        // Display Informational Message
        qDebug() << "Your account has been suspended!\nREASON: " << reason;

        // TODO: Need to send a notification to an admin

    }
    // Lets stop the application from running
    stopAndExitSlot();
}

/**
 * @brief slotAdminDeviceSuspend (SWITCHBOARD SIGNAL)
 * Administrator forced logout and reset received via switchboard. Essentially a device reset/suspension.
 */
void WickrIOClientMain::slotAdminDeviceSuspend()
{
    m_operation->postEvent("Device is suspended!", true);

    // Display Informational Message
    qDebug() << "System has reset this device as a result of either a forgot password performed by user, or a user account deletion.";

    // TODO: Need to send a notification to an admin

    // Lets stop the application from running
    stopAndExitSlot();
}

void WickrIOClientMain::slotSetSuspendError() {
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
void WickrIOClientMain::slotMessageDownloadStatusStart(int msgsToDownload)
{
    qDebug() << "Messages to download:" << msgsToDownload;
}

void WickrIOClientMain::slotMessageDownloadStatusUpdate(int msgsDownloaded)
{
    Q_UNUSED(msgsDownloaded);
}



/**
 * @brief WickrIOClientMain::pauseAndExitSlot
 * Call this slot to put the state of the client into the DOWN state,
 * and exit the client application.
 */
void WickrIOClientMain::stopAndExitSlot()
{
    stopAndExit(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOClientMain::pauseAndExitSlot
 * Call this slot to put the state of the client into the paused state,
 * and exit the client application.
 */
void WickrIOClientMain::pauseAndExitSlot()
{
    stopAndExit(PROCSTATE_PAUSED);
}

void WickrIOClientMain::slotReceivedMessage(QString type, QString value)
{
    if (type.toLower() == WBIO_IPCMSGS_PASSWORD) {
        // Check if we are waiting for the password to proceed with the login
        qDebug() << "Proceeding with the login";
        if (m_waitingForPassword) {
            m_waitingForPassword = false;

            QMap<QString,QString> qmapval = WickrIOIPCCommands::parsePasswordValue(value);
            if (qmapval.size() > 0) {
                m_password = qmapval.value(WBIO_BOTINFO_PASSWORD);
                m_loginHdlr.addLogin(m_user, m_password, m_userName, "");
                m_loginHdlr.initiateLogin();
            }
        }
    }
}


void WickrIOClientMain::processStarted()
{
    qDebug() << "Started WickrIOClientMain";

    if (! startTheClient())
        stopAndExit(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOClientMain::setIPC
 * Save the IPC object.  Only really need to make a connection to receive the stop
 * signal when the applicaiton is to be closed.
 * @param ipc
 */
void WickrIOClientMain::setIPC(WickrIOIPCService *ipc)
{
    m_wickrIPC = ipc;
    connect(ipc, &WickrIOIPCService::signalGotStopRequest, this, &WickrIOClientMain::stopAndExitSlot);
    connect(ipc, &WickrIOIPCService::signalGotPauseRequest, this, &WickrIOClientMain::pauseAndExitSlot);
    connect(ipc, &WickrIOIPCService::signalReceivedMessage, this, &WickrIOClientMain::slotReceivedMessage);
}

void WickrIOClientMain::slotDoTimerWork()
{
    // If there is a duration set then check if it is time to stop running
    if (m_operation->duration > 0) {
        if (m_durationreached)
            return;
        m_seccount++;
        if (m_seccount > m_operation->duration) {
            m_durationreached = true;
            QString exitString("Duration time has been reached, exiting!");
            m_operation->log_handler->log(exitString);
            m_operation->postEvent(exitString, false);
            emit signalExit();
            return;
        }
    }

    if (m_eventService != nullptr && !m_eventService->isHealthy()) {
        qDebug() << "Event Handler Service is NOT healthy!";
    }
    if (m_actionService != nullptr && !m_actionService->isHealthy()) {
        qDebug() << "Action Handler Service is NOT healthy!";
    }
    if (m_rxService != nullptr && !m_rxService->isHealthy()) {
        qDebug() << "Receive Service is NOT healthy!";
    }
}

/**
 * @brief WickrIOClientRuntime::slotServiceNotLoggedIn
 * This slot is called when a thread has been found to be unlogged
 * in for longer than a good period, so we will stop the client to
 * refresh things.
 */
void
WickrIOClientMain::slotServiceNotLoggedIn()
{
    m_operation->postEvent("Thread has not been able to log in for extended period of time.", false);

    stopAndExit(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOClientMain::stopAndExit
 * This is called to exit the application. The application state is put into the input
 * state, in the database process_state table.
 */
void WickrIOClientMain::stopAndExit(int procState)
{
    m_operation->postEvent("Shutting down", false);

    // Set the process state for after the shutdown
    WickrIOClientRuntime::wdSetShutdownState(procState);

    // logout of switchboard service
    WickrCore::WickrRuntime::swbSvcLogout();

    // logout to message service
    WickrCore::WickrRuntime::msgSvcLogout();

    // logout to task service (NOTE: only if closing application)
    WickrCore::WickrRuntime::taskSvcLogout();

//    WickrService::requestLogoff();

    // shutdown the receive service. will not return till it is down
    if (m_rxService != nullptr) {
        m_rxService->stopReceive();
        m_rxService->shutdown();
        m_rxService->deleteLater();
        m_rxDetails->deleteLater();
    }

    // shutdown the event handler service.  Will not return till it is down.
    if (m_eventService != nullptr) {
        m_eventService->shutdown();
        m_eventService->deleteLater();
    }

    // shutdown the action handler service.  Will not return till it is down.
    if (m_actionService != nullptr) {
        m_actionService->shutdown();
        m_actionService->deleteLater();
    }

    // If the logins have failed, make sure tbhe DB is open so the state can be changed
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
        // If we clean up messaging when logging out then do so here
        if (m_operation->m_cleanDBOnLogout) {
            WickrDBAdapter::clearUserAndMsgData();
        }

        if (WickrCore::WickrSession::getActiveSession()) {
            WickrCore::WickrSession::getActiveSession()->closeSession(true);
        }
        WickrDBAdapter::closeDB();
    }

    QCoreApplication::quit();
//    QCoreApplication::exit(1);
}

/**
 * @brief WickrIOClientMain::startTheClient
 * This method will start the client running, if it is not active already. The process
 * of starting the client begins with the login process.
 * @return  True is returned if the process is initiated. False if already active.
 */
bool WickrIOClientMain::startTheClient()
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
 * @brief WickrIOClientMain::slotSwitchboardState (SWITCHBOARD SIGNAL)
 * Slot accepts state change update signals from switchboard service
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOClientMain::slotSwitchboardServiceState(WickrServiceState state, SBSessionStatus sessionStatus, const QString& text)
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
        // If client state is LOGIN STARTED, we will force logout of switchboard, and
        // subsequently log client out.
        if (m_clientState == SERVICE_LOGIN_COMPLETE || text == "Login Timeout") {
            if (text == "Login Timeout")
                qDebug() << "Switchboard login has timed out, you are being logged out.";
            else
                qDebug() << "Switchboard currently unavailable, you are being logged out.";
            WickrCore::WickrRuntime::swbSvcLogout();
            logout();
        }
#endif

    default:
        break;
    }
}

/**
 * @brief WickrIOClientMain::slotMessageServiceState
 * @param state
 * @param text
 */
void WickrIOClientMain::slotMessageServiceState(WickrServiceState state)
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
 * @brief WickrIOClientMain::slotTaskServiceState
 * Slot accepts state change update signals from task service.
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOClientMain::slotTaskServiceState(WickrServiceState state)
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
 * @brief WickrIOClientMain::slotOnLoginMsgSynchronizationComplete()
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
void WickrIOClientMain::slotOnLoginMsgSynchronizationComplete()
{
    // ONLINE PROCESSING

    // Perform immediate houskeeping after download (backups if required, unsuspend convo backup after login)
    if (WickrCore::WickrRuntime::taskSvcIsConvoBackupEnabled()) {
        WickrCore::WickrRuntime::taskSvc()->suspendConvoBackUp(false);
        WickrConvoBackupContext *c = new WickrConvoBackupContext();
        WickrCore::WickrRuntime::taskSvcMakeRequest(c, true);
    } else {
        WickrCore::WickrRuntime::taskSvc()->suspendConvoBackUp(true);
    }

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


bool WickrIOClientMain::parseSettings(QSettings *settings)
{
    /*
     * Parse out the settings associated with the User
     */
    settings->beginGroup(WBSETTINGS_USER_HEADER);
    QString user = settings->value(WBSETTINGS_USER_USER, "").toString();
    QString password = settings->value(WBSETTINGS_USER_PASSWORD, "").toString();
    QString username = settings->value(WBSETTINGS_USER_USERNAME, "").toString();
    QString transactionID = settings->value(WBSETTINGS_USER_TRANSACTIONID, "").toString();
    bool autologin = settings->value(WBSETTINGS_USER_AUTOLOGIN, true).toBool();
    settings->endGroup();

    if (user.isEmpty()) {
        qDebug() << "User name is not set";
        return false;
    }

    // Save for use in main
    m_user = user;
    m_password = password;
    m_userName = username;

    // there is no password set
    if (password.isEmpty()) {
        // If we are using autologin check if the db password exists
        if (autologin) {
            // Check if the database password has been created.
            // If not then will need the client's password to start.
            QString clientDbDir = QString(WBIO_CLIENT_DBDIR_FORMAT).arg(WBIO_DEFAULT_DBLOCATION).arg(username);
            QString dbKeyFileName = QString("%1/dkd.wic").arg(clientDbDir);
            QFile dbKeyFile(dbKeyFileName);
            if (!dbKeyFile.exists()) {
                m_waitingForPassword = true;
                return true;
            }
        }
    }

    m_loginHdlr.addLogin(user, password, username, transactionID);
    m_waitingForPassword = false;

    /*
     * Parse out the settings associated with general configuration
     */
    settings->beginGroup(WBSETTINGS_CONFIG_HEADER);

    // Check for the DoReceive config is set
    m_operation->receiveOn = settings->value(WBSETTINGS_CONFIG_DORECEIVE, true).toBool();

    // Get the server name, if one is set
    m_serverName = settings->value(WBSETTINGS_CONFIG_SERVER, ClientConfigurationInfo::DefaultBaseURL).toString();

    // Set the Base URL for communications to the server
    WickrURLs::setBaseURL(m_serverName);

    settings->endGroup();
    return true;
}

