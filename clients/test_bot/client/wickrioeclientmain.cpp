#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "wickrbotsettings.h"
#include "wickrioeclientmain.h"
#include "wickriojson.h"
#include "wickrbotprocessstate.h"
#include "wickrbotutils.h"
#include "wickrbotactiondatabase.h"

#include "user/wickrUser.h"
#include "libinterface/libwickrcore.h"
#include "messaging/wickrInbox.h"
#include "messaging/wickrSecureRoomMgr.h"
#include "common/wickrRuntime.h"

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"

WickrIOEClientMain *WickrIOEClientMain::theBot;

/**
 * @brief WickrIOEClientMain::WickrIOEClientMain
 * This is the constructor for this class. Variables are initialized, the logins are created and
 * the m_logins list is set with those logins. Logging is started, counts initialized and SLOTS
 * are setup to receive specific SIGNALs
 */
WickrIOEClientMain::WickrIOEClientMain(OperationData *operation) :
    m_operation(operation),
    m_loginHdlr(operation),
    m_actionHdlr(operation),
    m_wickrIPC(0)
{
    if( isVERSIONDEBUG() ) {
        m_operation->cleanUpSecs = 20;
        m_operation->startRcvSecs = 2;
    } else {
        m_operation->cleanUpSecs = 300;
        m_operation->startRcvSecs = 2;
    }

    m_rxThread = new WickrIOReceiveThread(operation);

    this->connect(this, &WickrIOEClientMain::started, this, &WickrIOEClientMain::processStarted);
    this->connect(this, &WickrIOEClientMain::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

    this->connect(&m_actionHdlr, &WickrIOActionHdlr::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

    this->connect(&m_loginHdlr, &WickrIOLoginHdlr::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);
    this->connect(&m_loginHdlr, &WickrIOLoginHdlr::signalLoginSuccess, this, &WickrIOEClientMain::slotLoginSuccess);



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
                &WickrMessageService::signalDownloadAtLoginStart,
                this,
                &WickrIOEClientMain::slotMessageDownloadStatusStart);
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
        connect(roomMgr,
                &WickrCore::WickrSecureRoomMgr::signalDeleteRoom,
                this,
                &WickrIOEClientMain::slotDeleteRoom);
        connect(roomMgr,
                &WickrCore::WickrSecureRoomMgr::signalRemoveFromRoom,
                this,
                &WickrIOEClientMain::slotRemoveFromRoom);
#endif
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
        m_operation->updateProcessState(PROCSTATE_DOWN, false);
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

void WickrIOEClientMain::slotDeleteRoom(const QString& vGroupID, bool selfInitiated)
{
    Q_UNUSED(selfInitiated);

    WickrCore::WickrConvo* convo = WickrCore::WickrConvo::getConvoWithvGroupID( vGroupID );
    if (convo) {
        convo->dodelete(WickrCore::WickrConvo::DeleteInternal, false);
    }
}

void WickrIOEClientMain::slotRemoveFromRoom(const QString& vGroupID)
{
    WickrCore::WickrConvo* convo = WickrCore::WickrConvo::getConvoWithvGroupID( vGroupID );
    if (convo) {
        convo->dodelete(WickrCore::WickrConvo::DeleteInternal, false);
    }
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

    emit signalLoginSuccess();

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
    QMetaObject::invokeMethod(m_rxThread, "startReceiving", Qt::QueuedConnection);
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
void WickrIOEClientMain::slotAdminUserSuspend(const QString& reason)
{
    // Display Informational Message
    qDebug() << "You have been logged out of the system.\nREASON: " << reason;

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


void WickrIOEClientMain::processStarted()
{
    qDebug() << "Started WickrIOEClientMain";

    if (! startTheClient())
        stopAndExit(PROCSTATE_DOWN);
}

/**
 * @brief WickrIOEClientMain::setIPC
 * Save the IPC object.  Only really need to make a connection to receive the stop
 * signal when the applicaiton is to be closed.
 * @param ipc
 */
void WickrIOEClientMain::setIPC(WickrBotMainIPC *ipc)
{
    m_wickrIPC = ipc;
    connect(ipc, &WickrBotMainIPC::signalGotStopRequest, this, &WickrIOEClientMain::stopAndExitSlot);
    connect(ipc, &WickrBotMainIPC::signalGotPauseRequest, this, &WickrIOEClientMain::pauseAndExitSlot);
}

void WickrIOEClientMain::slotDoTimerWork()
{
    // If we are shutting down then do not proceed
    if (m_actionHdlr.isShuttingDown()) {
        return;
    }

    if (m_wickrIPC != NULL && m_wickrIPC->isRunning()) {
        m_wickrIPC->check();
    }

    m_actionHdlr.doTimerWork();
}

/**
 * @brief WickrIOEClientMain::stopAndExit
 * This is called to exit the application. The application state is put into the input
 * state, in the database process_state table.
 */
void WickrIOEClientMain::stopAndExit(int procState)
{
    // logout of switchboard service
    WickrCore::WickrRuntime::swbSvcLogout();

    // logout to message service
    WickrCore::WickrRuntime::msgSvcLogout();

    // logout to task service (NOTE: only if closing application)
    WickrCore::WickrRuntime::taskSvcLogout();

//    WickrService::requestLogoff();

    QMetaObject::invokeMethod(m_rxThread, "stopReceiving", Qt::QueuedConnection);

    m_actionHdlr.setShutowntime(true);
    m_actionHdlr.setShuttingDown(true);
    if (m_actionHdlr.isProcessingActions() ||
        m_actionHdlr.isProcessingCleanUp() ||
        m_rxThread->m_threadState != Idle) {
        // TODO: Need to make sure the callbackservice is done

        // Wait till the action/cleanup/rcvmsg processes are finished
        QTimer timer;
        QEventLoop loop;

        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

        int iterationsProcAction = 20;
        while ((m_actionHdlr.isProcessingActions() && iterationsProcAction > 0) ||
               m_actionHdlr.isProcessingCleanUp() ||
               m_rxThread->m_threadState != Idle ) {
            //TODO: need to make sure the callback service is stopped
            if (m_actionHdlr.isProcessingActions()) {
                iterationsProcAction--;
                qDebug() << "Waiting for client Action Handler process to finish!";
            }
            if (m_actionHdlr.isProcessingCleanUp()) {
                qDebug() << "Waiting for client Clean Up process to finish!";
            }
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

        if (m_operation->m_botDB->isOpen()) {
            m_operation->updateProcessState(procState, false);
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

    m_operation->log("Starting");

    // Open the database if needed and get the Client information
    if (m_operation->m_botDB == NULL) {
        m_operation->m_botDB = new WickrIOClientDatabase(m_operation->databaseDir);
    }

    // TASK SERVICE: Utility service login here (to ensure rest api available)
    WickrCore::WickrRuntime::taskSvcLogin();

    m_loginHdlr.initiateLogin();

    emit signalStarted();
    return true;
}

/**
 * @brief WickrIOEClientMain::slotSwitchboardState (SWITCHBOARD SIGNAL)
 * Slot accepts state change update signals from switchboard service
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotSwitchboardServiceState(WickrServiceState state, const QString& text)
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

    if (user.isEmpty() || password.isEmpty()) {
        qDebug() << "User or password is not set";
        return false;
    }
    m_loginHdlr.addLogin(user, password);

    settings->endGroup();

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

