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

#include "clientconfigurationinfo.h"
#include "clientversioninfo.h"

#include "wickrIOClientRuntime.h"
#include <QJsonDocument>
#include "libinterface/libwickrcore.h"

WickrIOEClientMain *WickrIOEClientMain::theBot;

/**
 * @brief WickrIOEClientMain::WickrIOEClientMain
 * This is the constructor for this class. Variables are initialized, the logins are created and
 * the m_logins list is set with those logins. Logging is started, counts initialized and SLOTS
 * are setup to receive specific SIGNALs
 */
WickrIOEClientMain::WickrIOEClientMain(WickrIOClients *client, const QString& invite) :
    m_client(client),
    m_invite(invite)
{
    this->connect(this, &WickrIOEClientMain::started, this, &WickrIOEClientMain::processStarted);
    this->connect(this, &WickrIOEClientMain::signalExit, this, &WickrIOEClientMain::stopAndExitSlot);

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
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminUserSuspend,
                this,
                &WickrIOEClientMain::slotAdminUserSuspend);
        connect(swbsvc,
                &WickrSwitchboardService::signalAdminDeviceSuspend,
                this,
                &WickrIOEClientMain::slotAdminDeviceSuspend);

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

    QCoreApplication::processEvents();
}


/**
 * @brief WickrIOEClientMain::processStarted
 * This slot is called when the thread starts up
 */
void WickrIOEClientMain::processStarted()
{
    qDebug() << "Started WickrIOEClientMain";

    // TASK SERVICE: Utility service login here (to ensure rest api available)
    WickrCore::WickrRuntime::taskSvcLogin();

    emit signalStarted();

    // connect to signals from the provisioning handler
    WickrIOProvisionHdlr *provhdlr = WickrIOClientRuntime::provHdlr();
    if (provhdlr) {
        connect(provhdlr,
                &WickrIOProvisionHdlr::signalPageChanged,
                this,
                &WickrIOEClientMain::slotProvisionPageChanged);

        connect(provhdlr,
                &WickrIOProvisionHdlr::signalRegisterOnPrem,
                &m_loginHdlr,
                &WickrIOLoginHdlr::slotRegisterOnPrem);
        connect(provhdlr,
                &WickrIOProvisionHdlr::signalRegisterEnterprise,
                &m_loginHdlr,
                &WickrIOLoginHdlr::slotRegisterUser);
    }

    // Start the provisioning here
    if (m_client->onPrem) {
        WickrIOClientRuntime::provHdlrBeginOnPrem(m_client->user, m_client->password, m_invite);
    } else {
        WickrIOClientRuntime::provHdlrBeginCloud(m_client->user, m_invite);
    }
}

void WickrIOEClientMain::slotProvisionPageChanged(WickrIOProvisionHdlr::Page page)
{
    WickrIOProvisionHdlr *provhdlr = WickrIOClientRuntime::provHdlr();
    if (!provhdlr)
        return;

    switch (page)
    {
    case WickrIOProvisionHdlr::enterEmail:
        qDebug() << "enterEmail";
        break;
    case WickrIOProvisionHdlr::verifyEmail:
        qDebug() << "verifyEmail";
        break;
    case WickrIOProvisionHdlr::enterPhone:
        qDebug() << "enterPhone";
        break;
    case WickrIOProvisionHdlr::verifyPhone:
        qDebug() << "verifyPhone";
        break;
    case WickrIOProvisionHdlr::askVideoPermission:
        qDebug() << "askVideoPermission";
        break;
    case WickrIOProvisionHdlr::recordVideo:
        qDebug() << "recordVideo";
        break;
    case WickrIOProvisionHdlr::enterPassword:
        qDebug() << "enterPassword";
        if (m_client->onPrem) {
            // Generate the password
            m_client->password = WickrIOTokens::getRandomString(24);
            qDebug() << "CONSOLE:********************************************************************";
            qDebug() << "CONSOLE:**** GENERATED PASSWORD";
            qDebug() << "CONSOLE:**** DO NOT LOSE THIS PASSWORD, YOU WILL NEED TO ENTER IT EVERY TIME";
            qDebug() << "CONSOLE:**** TO START THE BOT";
            qDebug() << "CONSOLE:****";
            qDebug() << "CONSOLE:" << m_client->password;
            qDebug() << "CONSOLE:********************************************************************";
            provhdlr->registerWithPassword(m_client->password);
        } else {
            provhdlr->setPassword(m_client->password);

            // At this point login
            m_loginHdlr.addLogin(m_client->user, m_client->password, m_client->name, provhdlr->transactionID(), true);
            m_loginHdlr.initiateLogin();
        }
        break;
    case WickrIOProvisionHdlr::askContactsPermission:
        qDebug() << "askContactsPermission";
        break;
    case WickrIOProvisionHdlr::askJoinNetwork:
        qDebug() << "askJoinNetwork";
        break;
    }
}

/**
 * @brief WickrIOEClientMain::slotLoginSuccess
 * This slot is called when the login is successful
 */
void WickrIOEClientMain::slotLoginSuccess()
{
    QByteArray userSKey = m_loginHdlr.getSigningKey();
    qDebug() << "CONSOLE:********************************************************************";
    qDebug() << "CONSOLE:**** USER SIGNING KEY";
    qDebug() << "CONSOLE:**** You will need this to enter into the console for the Bot";
    qDebug() << "CONSOLE:****";
    qDebug() << "CONSOLE:" << QString(userSKey.toHex());
    qDebug() << "CONSOLE:********************************************************************";

    emit signalLoginSuccess();
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

    if (m_loginHdlr.getLoginState() == LoggedIn) {
        if (WickrCore::WickrSession::getActiveSession()) {
            WickrCore::WickrSession::getActiveSession()->closeSession(true);
        }
        WickrDBAdapter::closeDB();
    }

    QCoreApplication::quit();
//    QCoreApplication::exit(1);
}

void WickrIOEClientMain::slotInitiateLogin()
{
    m_loginHdlr.initiateLogin();
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

void
WickrIOEClientMain::loadBootstrapFile(const QString& fileName, const QString& passphrase)
{
    qDebug() << "The bootstrap file " << fileName << " with the passphrase " << passphrase << " was loaded";

    QFile bootstrap(fileName);

    // Try to decrypt the configuration file
    if (!bootstrap.open(QIODevice::ReadOnly))
    {
        qDebug() << "can't open bootstrap file";
        return;
    }
    QByteArray bootstrapBlob(bootstrap.readAll());
    bootstrap.close();
    if (bootstrapBlob.size() == 0)
        return;

    QString bootstrapStr = cryptoDecryptConfiguration(passphrase, bootstrapBlob);
    if (bootstrapStr == NULL || bootstrapStr.isEmpty()) {
        if (!bootstrap.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "can't open bootstrap file";
            return;
        }
        // try reading the file as a text file
        bootstrapStr = QString(bootstrap.readAll());
        bootstrap.close();
        if (bootstrapStr == NULL || bootstrapStr.isEmpty())
            return;
    }

    QJsonDocument d;
    d = d.fromJson(bootstrapStr.toUtf8());
    if(WickrCore::WickrRuntime::getEnvironmentMgr()->loadBootStrapJson(d))
    {
//        WickrCore::WickrRuntime::getEnvironmentMgr()->storeNetworkConfig(controller->getNetworkSettings());
    } else {
        qDebug() << "Incorrect credentials - please try again. Configuration file error";
    }
}

