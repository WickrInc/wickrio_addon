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

    m_callbackThread = new WickrIOCallbackThread(operation);
    m_rxThread = new WickrIOReceiveThread(operation);

    this->connect(this, &WickrIOEClientMain::started, this, &WickrIOEClientMain::processStarted);
    this->connect(this, &WickrIOEClientMain::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

    this->connect(&m_actionHdlr, &WickrIOActionHdlr::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

    this->connect(&m_loginHdlr, &WickrIOLoginHdlr::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);
    this->connect(&m_loginHdlr, &WickrIOLoginHdlr::signalLoginSuccess, this, &WickrIOEClientMain::slotLoginSuccess);












    WickrSwitchboardThread *switchboard = WickrCore::WickrRuntime::getSwitchboard();
    WickrMessageSendThread *messagesend = WickrCore::WickrRuntime::getMessageSend();
    WickrMessageCheckThread *messagecheck = WickrCore::WickrRuntime::getMessageCheck();
    WickrMessageDownloadThread *messagedownload = WickrCore::WickrRuntime::getMessageDownload();
    WickrMessageCommitThread *messagecommit= WickrCore::WickrRuntime::getMessageCommit();
    WickrTaskThread *tasksvc = WickrCore::WickrRuntime::getTaskThread();

    if (switchboard && messagecheck && messagedownload && messagecommit && tasksvc) {
        // Signals to switchboard (from wickrMain for Login,LoginUpdate,Logout)
        connect(this,
                &WickrIOEClientMain::signalLoginSwitchboard,
                switchboard,
                &WickrSwitchboardThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutSwitchboard,
                switchboard,
                &WickrSwitchboardThread::slotLogout);

#if 0
        // Signals to switchboard (from convomodels for signalConvoDevicSync)
        connect(secureRoomModel,
                &ConvoModel::signalConvoDeviceSync,
                switchboard,
                &WickrSwitchboardThread::slotReportDeviceSync);
        connect(oneToOneModel,
                &ConvoModel::signalConvoDeviceSync,
                switchboard,
                &WickrSwitchboardThread::slotReportDeviceSync);
#endif

        // Signals from switchboard (StateChange,MessageCheck)
        connect(switchboard,
                &WickrSwitchboardThread::signalState,
                this,
                &WickrIOEClientMain::slotSwitchboardState);
#if 0
        connect(switchboard,
                &WickrSwitchboardThread::signalUserVideoUpdate,
                controller,
                &wickrMain::slotUserVideoUpdate);
        connect(switchboard,
                &WickrSwitchboardThread::signalAdminForcedLogout,
                controller,
                &wickrMain::slotAdminForcedLogout);
#endif

        // Signals to message service(message send) (from wickrMain for Login,Logout,MessageSend)
        connect(this,
                &WickrIOEClientMain::signalLoginMessageService,
                messagesend,
                &WickrMessageSendThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutMessageService,
                messagesend,
                &WickrMessageSendThread::slotLogout);
        connect(&m_actionHdlr,
                &WickrIOActionHdlr::signalMessageSend,
                messagesend,
                &WickrMessageSendThread::slotMessageSendRequest);

#if 0
        // Signals from message service(message send) (StateChange, Network Status, Suspend)
        connect(messagesend,
                &WickrMessageSendThread::signalState,
                controller,
                &wickrMain::slotMessageSendState);
        connect(messagesend,
                &WickrMessageSendThread::signalReceivedSuspendedAccountError,
                controller,
                &wickrMain::slotSetSuspendError);
#endif

        // Signals to message service(message check) (from wickrMain for Login,Logout,MessageCheck)
        connect(this,
                &WickrIOEClientMain::signalLoginMessageService,
                messagecheck,
                &WickrMessageCheckThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutMessageService,
                messagecheck,
                &WickrMessageCheckThread::slotLogout);
        connect(this,
                &WickrIOEClientMain::signalMessageCheck,
                messagecheck,
                &WickrMessageCheckThread::slotMessageCheckRequest);

        // Signals from message service(message check) (StateChange, Network Status)
        connect(messagecheck,
                &WickrMessageCheckThread::signalState,
                this,
                &WickrIOEClientMain::slotMessageCheckState);
        connect(messagecheck,
                &WickrMessageCheckThread::signalDownloadAtLoginEnd,
                this,
                &WickrIOEClientMain::slotOnLoginMsgSynchronizationComplete);

        connect(messagecheck, &WickrMessageCheckThread::signalReceivedSuspendedAccountError, this, [=]
        {
//            emit sigLogout();
            qDebug() << "Your Wickr ID has been suspended. If you feel this is in error please contact us by email at support@wickr.com";
        });



        // Signals to message service(message download) (from wickrMain for Login,Logout)
        connect(this,
                &WickrIOEClientMain::signalLoginMessageService,
                messagedownload,
                &WickrMessageDownloadThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutMessageService,
                messagedownload,
                &WickrMessageDownloadThread::slotLogout);

        // Signals from message service(message download) (StateChange)
        connect(messagedownload,
                &WickrMessageDownloadThread::signalState,
                this,
                &WickrIOEClientMain::slotMessageDownloadState);
#if 0
        connect(messagedownload,
                &WickrMessageDownloadThread::signalDownloadAtLoginStart,
                this,
                &wickrQuickMain::slotMessageDownloadStatusStart);
#endif

        // Signals to message service(message commit) (from wickrMain for Login,Logout)
        connect(this,
                &WickrIOEClientMain::signalLoginMessageService,
                messagecommit,
                &WickrMessageCommitThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutMessageService,
                messagecommit,
                &WickrMessageCommitThread::slotLogout);

        // Signals from message service(message commit) (StateChange, DecodeError)
        connect(messagecommit,
                &WickrMessageCommitThread::signalState,
                this,
                &WickrIOEClientMain::slotMessageCommitState);
#if 0
        connect(messagecommit,
                &WickrMessageCommitThread::signalDownloadAtLoginUpdate,
                this,
                &wickrQuickMain::slotMessageDownloadStatusUpdate);
#endif
        connect(messagecommit,
                &WickrMessageCommitThread::signalDownloadAtLoginEnd,
                this,
                &WickrIOEClientMain::slotOnLoginMsgSynchronizationComplete);


        // Signals from message service(message commit) (StateChange, DecodeError)
        connect(tasksvc,
                &WickrTaskThread::signalState,
                this,
                &WickrIOEClientMain::slotTaskServiceState);

        connect(this,
                &WickrIOEClientMain::signalLoginTaskService,
                tasksvc,
                &WickrTaskThread::slotLogin);
        connect(this,
                &WickrIOEClientMain::signalLogoutTaskService,
                tasksvc,
                &WickrTaskThread::slotLogout);
        connect(&m_loginHdlr,
                &WickrIOLoginHdlr::signalMakeRequest,
                tasksvc,
                &WickrTaskThread::slotMakeRequest);
        connect(&m_actionHdlr,
                &WickrIOActionHdlr::signalMakeRequest,
                tasksvc,
                &WickrTaskThread::slotMakeRequest);
        connect(this,
                &WickrIOEClientMain::signalMakeJob,
                tasksvc,
                &WickrTaskThread::slotMakeJob);
        connect(this,
                &WickrIOEClientMain::signalDatabaseLoadRequest,
                tasksvc,
                &WickrTaskThread::slotDatabaseLoadRequest);
        qDebug() << "MESSAGE SERVICES: Application connections initialized.";

    } else {
        if (!switchboard || !messagecheck || !messagedownload || !messagecommit)
            qDebug() << "initMessageServicesConnections(): MESSAGE SERVICES - Not initialized.";
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
    WickrTaskThread *taskThread = WickrCore::WickrRuntime::getTaskThread();
    connect(taskThread, &WickrTaskThread::signalDatabaseLoadComplete, this, &WickrIOEClientMain::slotDatabaseLoadDone, Qt::QueuedConnection);
    emit signalDatabaseLoadRequest(WickrUtil::dbDump);
    emit signalLoginSuccess();

    // Update switchboard login credentials (login is performed only if not already logged in)
    emit signalLoginSwitchboard(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                WickrCore::WickrSession::getActiveSession()->getAppID(),
                                WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                true);

    // Start the callback thread
    m_callbackThread->start();

    // Start the receive thread
    connect(m_rxThread, &WickrIOReceiveThread::signalProcessStarted, this, &WickrIOEClientMain::slotRxProcessStarted, Qt::QueuedConnection);
    m_rxThread->start();

    // ONLINE: Login successful, so login to message service
    emit signalLoginMessageService();
}

/**
 * @brief WickrIOEClientMain::slotDatabaseLoadDone
 * Complete database load.
 */
void WickrIOEClientMain::slotDatabaseLoadDone()
{
    WickrTaskThread *taskThread = WickrCore::WickrRuntime::getTaskThread();
    disconnect(taskThread, &WickrTaskThread::signalDatabaseLoadComplete, this, &WickrIOEClientMain::slotDatabaseLoadDone);

    startTimer();
#if 0
    // Emit update signals
    emit signalUserListUpdated();

    // Complete user setup as part of login
    processUserSetup();
#endif
}

void WickrIOEClientMain::slotRxProcessStarted()
{
    QMetaObject::invokeMethod(m_rxThread, "startReceiving", Qt::QueuedConnection);
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
    emit signalLogoutSwitchboard();

    // logout to message service
    emit signalLogoutMessageService();

    // logout to task service (NOTE: only if closing application)
    emit signalLogoutTaskService();

//    WickrService::requestLogoff();

    QMetaObject::invokeMethod(m_rxThread, "stopReceiving", Qt::QueuedConnection);

    m_actionHdlr.setShutowntime(true);
    m_actionHdlr.setShuttingDown(true);
    if (m_actionHdlr.isProcessingActions() ||
        m_actionHdlr.isProcessingCleanUp() ||
        m_rxThread->m_threadState != Idle ||
        m_callbackThread->m_threadState != Idle) {
        // Wait till the action/cleanup/rcvmsg processes are finished
        QTimer timer;
        QEventLoop loop;

        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

        int iterationsProcAction = 20;
        while ((m_actionHdlr.isProcessingActions() && iterationsProcAction > 0) ||
               m_actionHdlr.isProcessingCleanUp() ||
               m_rxThread->m_threadState != Idle  ||
               m_callbackThread->m_threadState != Idle) {
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
            if ( m_callbackThread->m_threadState != Idle) {
                qDebug() << "Waiting for Callback Thread process to finish!";
                QMetaObject::invokeMethod(m_callbackThread, "stopProcessing", Qt::QueuedConnection);
            }

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
    emit signalLoginTaskService();

    m_loginHdlr.initiateLogin();

    emit signalStarted();
    return true;
}

/**
 * @brief wickrMain::slotTaskServiceState
 * Slot accepts state change update signals from task service.
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotTaskServiceState(WickrTaskThread::ThreadState state,
                                              const QString& text)
{
    Q_UNUSED(text);
    switch(state) {
    case WickrTaskThread::LOGGED_IN:
        WickrCore::WickrSession::serviceLoggedIn("TaskService");
        break;
    case WickrTaskThread::LOGGED_OUT:
        WickrCore::WickrSession::serviceLoggedOut("TaskService");
        break;
    default:
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotSwitchboardState
 * Slot accepts state change update signals from switchboard service.
 * @param state
 */
void WickrIOEClientMain::slotSwitchboardState(WickrSwitchboardThread::ThreadState state, const QString& text)
{
    switch(state) {
    case WickrSwitchboardThread::LOGGED_IN:
        // Finish login
//        completePostLogin();
        break;

    case WickrSwitchboardThread::LOGGED_OUT:  // <- Logged Out will ALWAYS transition to DISCONNECTED
        // Intermediate states, no action taken
        break;

    case WickrSwitchboardThread::DISCONNECTED:
    default:
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotMessageCheckState
 * Slot accepts state change update signals from message service (message check thread).
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotMessageCheckState(WickrMessageCheckThread::ThreadState state, const QString& text)
{
    Q_UNUSED(text);

    switch(state) {
    case WickrMessageCheckThread::LOGGED_IN:
    case WickrMessageCheckThread::LOGGED_OUT:
        break;
    default:
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotMessageDownloadState
 * Slot accepts state change update signals from message service (message download thread).
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotMessageDownloadState(WickrMessageDownloadThread::ThreadState state, const QString& text)
{
    Q_UNUSED(text);
    switch(state) {
    case WickrMessageDownloadThread::LOGGED_IN:
    case WickrMessageDownloadThread::LOGGED_OUT:
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
    WickrCore::WickrRuntime::getTaskThread()->suspendConvoBackUp(false);
    WickrConvoBackupContext *c = new WickrConvoBackupContext();
    c->setAutoDelete(true); // No processing of response, Task Service will automatically delete.
    emit signalMakeRequest(c);

    // Start housekeeping timer (i.e. periodic backup check on interval)
//    housekeepingTimerStart();

    // After initial download complete, login to switchboard
    emit signalLoginSwitchboard(WickrCore::WickrSession::getActiveSession()->getSwitchboardServer(),
                                WickrCore::WickrUser::getSelfUser()->getServerIDHash(),
                                WickrCore::WickrSession::getActiveSession()->getAppID(),
                                WickrCore::WickrSession::getActiveSession()->getSwitchboardToken(),
                                false);
}

/**
 * @brief WickrIOEClientMain::slotMessageCommitState
 * Slot accepts state change update signals from message service (message commit thread).
 * Can be used to synchronize startup/shutdown procedures.
 * @param state
 */
void WickrIOEClientMain::slotMessageCommitState(WickrMessageCommitThread *thread,
                                       WickrMessageCommitThread::ThreadState state,
                                       const QString& text)
{
    Q_UNUSED(text);
    switch(state) {
    case WickrMessageCommitThread::LOGGED_IN:
        WickrCore::WickrSession::serviceLoggedIn("MessageService");

        // Perform initial message check
        emit signalMessageCheck(ON_LOGIN);
        break;

    case WickrMessageCommitThread::LOGGED_OUT:

        // Let's close the database only when the message service is completely logged out
        // This needs to be done by the WickrSession::closeSession(), which should
        // have a way to know the "message service" is done
        WickrCore::WickrSession::serviceLoggedOut("MessageService");
        break;
    default:
        break;
    }
}





bool WickrIOEClientMain::parseSettings(QSettings *settings)
{
    /*
     * Parse out the settings associated with the User
     */
    settings->beginGroup(WBSETTINGS_USER_HEADER);

    QString username = settings->value(WBSETTINGS_USER_USER, "").toString();
    QString password = settings->value(WBSETTINGS_USER_PASSWORD, "").toString();

    if (username.isEmpty() || password.isEmpty()) {
        qDebug() << "User or password is not set";
        return false;
    }
    m_loginHdlr.addLogin(username, password);

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

