#include "wickrIOClientLoginHdlr.h"

#include "filetransfer/wickrCloudTransferMgr.h"
#include "session/wickrRegistrationCtx.h"
#include "common/wickrRuntime.h"

WickrIOClientLoginHdlr::WickrIOClientLoginHdlr(OperationData *operation, int loginVersion) :
    m_operation(operation),
    m_curLoginIndex(0),
    m_loginState(LoggedOut),
    m_consecutiveLoginFailures(0),
    m_backupVersion(-1),
    m_preRegDataIface(nullptr),
    m_loginVersion(loginVersion)
{
    m_firstLogin = ! WickrDBAdapter::doesDBExist();
}

WickrIOClientLoginHdlr::~WickrIOClientLoginHdlr()
{
    if (this->m_loginState == LoggedIn) {
    }
}

/**
 * @brief wickrMain::preRegistrationInit
 * Call this function to initialize, or cleanup the Pre-Registration data. A call to this
 * function will initialize or reset the user's signing key.
 */
void WickrIOClientLoginHdlr::preRegistrationInit()
{
    if (m_preRegDataIface != NULL) {
        m_preRegDataIface->refresh();
    } else {
        m_preRegDataIface = new WickrCore::WickrPreRegistrationIface();
    }
}

/**
 * @brief wickrMain::preRegistrationGetKeyStrings
 * Call this function to get the list of strings that are associated with the Pre-Registration
 * User signing key.
 * @return List of strings that can be displayed during key verification video
 */
QStringList WickrIOClientLoginHdlr::preRegistrationGetKeyStrings()
{
    QStringList strings;

    if (m_preRegDataIface == NULL) {
        preRegistrationInit();
    }

    if (m_preRegDataIface != NULL) {
        QByteArray signKey = m_preRegDataIface->getUserSigningKey();

        if (WickrCore::WickrRuntime::getKeyVerifyMgr()) {
            strings = WickrCore::WickrRuntime::getKeyVerifyMgr()->getKeyVerificationWords(signKey);
        }
    }
    return strings;
}

/**
 * @brief wickrMain::slotRegisterEnterprise
 * @param wickrid
 * @param password
 * @param transactionid
 * @param sync
 * @param isRekey
 */
void WickrIOClientLoginHdlr::registerUser(const QString &wickrid, const QString &password, const QString &transactionid, bool newUser, bool sync, bool isRekey)
{
    WickrEnterpriseRegistrationParameters regParams(wickrid, transactionid, NULL);
    WickrPreRegistrationData *preRegData;
    if (m_preRegDataIface != NULL) {
        preRegData = m_preRegDataIface->getPreRegData();
    } else {
        preRegData = NULL;
    }

    WickrRegisterUserContext *c = new WickrRegisterUserContext(wickrid, password, QString(), WickrUtil::getDeviceIdentifier(), WickrUtil::getDeviceHost(), newUser, sync, isRekey, regParams, preRegData);
    c->putArg(arg_USERID,   wickrid );
    c->putArg(arg_PASSWORD, password );
    connect(c, &WickrRegisterUserContext::signalRequestCompleted,
            this, &WickrIOClientLoginHdlr::slotRegistrationDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOClientLoginHdlr::initiateLogin
 * This method will initiate a login using the credentials for one of the users from the
 * m_logins list variable. The index of the login to use is identified by the m_curLoginIndex
 * variable. The WickrRegistrationService will be called to initiate the login.
 * The database name will be set to the name of the user to be logged in.
 */
void WickrIOClientLoginHdlr::initiateLogin()
{
    if (m_consecutiveLoginFailures >= m_logins.size()) {
        m_loginState = LoginsFailed;
        emit signalExit();
    } else {
        m_loginState = InProcess;
        QString userid = m_logins.at(m_curLoginIndex)->m_name;
        QString password = m_logins.at(m_curLoginIndex)->m_pass;
        QString userName = m_logins.at(m_curLoginIndex)->m_userName;
        bool creatingUser = m_logins.at(m_curLoginIndex)->m_creating;

        if (m_operation->m_botDB->isOpen()) {
            EnvironmentMgr *env = WickrCore::WickrRuntime::getEnvironmentMgr();
            if (env->emailAsUserIdMode()) {
                m_operation->m_client = m_operation->m_botDB->getClientUsingUserName(userid);
            } else {
                m_operation->m_client = m_operation->m_botDB->getClientUsingName(userName);
            }
        }

        WickrDBAdapter::setDBNameForUser( userid );

        if (creatingUser) {
            // If the database already exists then cannot do a Registration!
            if( WickrDBAdapter::doesDBExist() ) {
                m_operation->log_handler->log(QString("Creating user will fail because DB already exists!"));
                emit signalExit();
            } else {
                m_operation->log_handler->log(QString("Starting registration to create user " + userid));

                QString transID;
                registerUser(userid, password, transID, true, false, false);
            }
        } else {
            // If the database exists then just login
            if (WickrDBAdapter::doesDBExist()) {
                m_operation->log_handler->log(QString("Starting login for user " + userid));

                slotLoginStart(userid, password);
            } else {
                QString transID;
                registerUser(userid, password, transID, false, true, false);
            }
        }
    }
}

/**
 * @brief WickrIOClientLoginHdlr::slotRegistrationDone
 * This SLOT is called when the registration service is complete. A login request will be made to
 * login the user identified by the userid and password variables.
 * @param ws
 */
void WickrIOClientLoginHdlr::slotRegistrationDone(WickrRegisterUserContext *c)
{
    m_operation->log_handler->log("Received registration");

    if(!c->isSuccess()) {
        // If we failed because of something other than bad credentials then show the result
        if (c->apiCode().getValue() != BAD_SYNC_CREDENTIALS) {
            m_operation->log_handler->log(c->errorString());
            emit signalOnlineFlag(false);
        }
    } else {
        // GET Arguments: <wickrid> <password>
        QString wickrid  = c->getArg(arg_USERID).toString();
        QString password = c->getArg(arg_PASSWORD).toString();
        slotLoginStart( wickrid, password );
    }
    c->deleteLater();
}

void WickrIOClientLoginHdlr::slotLoginStart(const QString& username, const QString& password)
{
    // Close database if already open
    WickrDBAdapter::closeDB();

    //qDebug() << "********* slotLoginStart(), called, before db check";
    if (!WickrDBAdapter::doesDBExist()) {
        if (WickrCore::WickrRuntime::getEnvironmentMgr()->hasConfiguration()) {
            QString networkToken = WickrCore::WickrRuntime::getEnvironmentMgr()->networkToken();
            if (networkToken.isEmpty() || username.isEmpty() || password.isEmpty()) {
                qDebug() << "ERROR: Either the network token, username, or password is empty";
                return;
            }

            // TODO: Do we need to handle this?
        } else {
            // No DB or wrong DB so try existing user registration
            QString unused = "UNUSED";
            QString user = username;
            QString pass = password;
            registerUser(user, pass, unused, false, true, false);
        }
        return;
    }

    QString otp="";
    WickrLoginContext *c = new WickrLoginContext(username,password,otp,m_loginVersion);
    connect(c, &WickrLoginContext::signalRequestCompleted, this, &WickrIOClientLoginHdlr::slotLoginDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOClientLoginHdlr::slotLoginDone
 * This SLOT is called when the login service completes. If the login is successful then the
 * timer will be started so that processing can begin. If the login fails then the program will
 * be exited.
 * TODO: If the login fails we need to determine why. Maybe clear the database.
 *
 * @param ws Pointer to the login service object.
 */
void WickrIOClientLoginHdlr::slotLoginDone(WickrLoginContext *ls)
{
    if (ls->isSuccess()) {
        m_operation->log_handler->log("Login successful");
        m_loginState = LoggedIn;
        m_consecutiveLoginFailures = 0;

        qDebug() << "Dumping Wickr database counts:";
        qDebug() << WickrDBAdapter::dumpTableCounts();

        QString s3_bucketId = ls->s3BucketId();
        QString s3_secret = ls->s3Secret();
        QString s3_key = ls->s3Key();
        QString s3_url = ls->s3Url();
        qDebug() << "Amazon S3 info: " << s3_bucketId << s3_key << s3_secret << s3_url;

        if (! WickrCore::WickrRuntime::ftSvcLogin(s3_bucketId, s3_key, s3_secret, s3_url )) {
            qDebug() << "Amazon S3 File Transfer login failed!";
        }

        m_backupVersion = -1;

#if 0 // TODO
        connect(WickrCore::WickrSession::getActiveSession()->getContactManager(), &WickrCore::WickrContactMan::contactDownloadComplete, this, [=]() {
            m_backupVersion = WickrCore::WickrSession::getActiveSession()->account->getBackupVersion();

            // Download complete, now refresh directory
            refreshDirectory();
        });
#endif

        // If contact back is enabled then initiate a restore
        if (WickrCore::WickrRuntime::taskSvcIsContactBackupEnabled()) {
            if (!WickrCore::WickrSession::getActiveSession()->getContactManager()->restoreContactsIfRequired( m_backupVersion )) {
                // No contact backup download needed, proceed
                refreshDirectory();
            }
        }

        // Store switchboard credentials from login receipt (in WickrSession)
        WickrCore::WickrSession::getActiveSession()->setSwitchboardServer(ls->switchboardServer());
        WickrCore::WickrSession::getActiveSession()->setSwitchboardToken(ls->switchboardToken());

        // Store networkId (in WickrSession)
        WickrCore::WickrSession::getActiveSession()->setNetworkIdFromLogin(ls->networkId());

        emit signalLoginSuccess();
    } else {
        m_loginState = LoggedOut;

        // If this is an attempt to create the user then fail
        if (m_logins.at(m_curLoginIndex)->m_creating) {

            /* The login failed, should we reset the database?
             */
            m_operation->log_handler->error("Login failed!");
            m_operation->log_handler->error("Program exiting!");
            m_operation->log_handler->error(ls->errorString());


            m_consecutiveLoginFailures++;
            m_logins.at(m_curLoginIndex)->m_failedLogin++;

            // Failed, try the next user if there is one
            loginNextUser();
        } else {
            /*
             * If okTorRunWithoutServer is true then login failed to the server but the local
             * password is correct.  So do NOT attempt to register which will fail.
             */
            if (ls->okToRunWithoutServer()) {
                m_operation->log_handler->error("Login failed to server, but local login was success!");
                m_operation->log_handler->error("Program exiting!");
                m_operation->log_handler->error(ls->errorString());

                m_consecutiveLoginFailures++;
                m_logins.at(m_curLoginIndex)->m_failedLogin++;

                ls->deleteLater();

                // Need to close the client database, because it is left open
                if (WickrCore::WickrSession::getActiveSession()) {
                    WickrCore::WickrSession::getActiveSession()->closeSession(true);
                }

                // Failed, try the next user if there is one
                loginNextUser();
                return;
            } else {
                // The initial login failed, lets try to create the user
                m_logins.at(m_curLoginIndex)->m_creating = true;
                initiateLogin();
            }
        }
    }

    ls->deleteLater();
}

void WickrIOClientLoginHdlr::refreshDirectory()
{
    // Refresh the directory
    // NOTE: We will transition from RELOGIN_STARTED to RELOGIN_COMPLETE at the end of this
    //       retrieval of directory and profiles (via completeLogin())
    WickrDirectoryGetContext *c = new WickrDirectoryGetContext(0);
    connect(c, &WickrDirectoryGetContext::signalRequestCompleted,
            this, &WickrIOClientLoginHdlr::slotRefreshDirectoryDone);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

// TODO: NEED TO GET THE BACKUP
void WickrIOClientLoginHdlr::slotRefreshDirectoryDone(WickrDirectoryGetContext* context)
{
    if (context->isSuccess()) {
        if (context->usersToValidate().size()) {
            WickrUserValidateUpdate *u = new WickrUserValidateUpdate(context->usersToValidate(),false,false,true,0);
            WickrCore::WickrRuntime::taskSvcMakeRequest(u);
        }

        if (context->updatedUsers().size()) {
            // DIRECTORY UPDATE: So now download profiles

            // Connect signals from Task Service
#if 0 // TODO
            WickrProfileJobContext *job = new WickrProfileJobContext(0,context->updatedUsers(),context->needsBackup());
            connect(job, &WickrProfileJobContext::signalJobCompleted,
                    this, &wickrMain::slotRefreshUserProfilesDone, Qt::QueuedConnection);
            connect(job, &WickrProfileJobContext::signalProfileImageUpdated,
                    this, &wickrMain::slotGetProfileImageUpdated, Qt::QueuedConnection);
            // Execute BATCH request (signal)
            WickrCore::WickrRuntime::taskSvcMakeRequest(job);
#endif
        } else {
            // NO DIRECTORY UPDATE: So just continue with direct call
//NOT NEEDED FOR IO?            slotRefreshUserProfilesDone(nullptr);
        }
    } else {
        // NO DIRECTORY UPDATE: So just continue with direct call
//NOT NEEDED FOR IO?        slotRefreshUserProfilesDone(nullptr);
    }
    context->deleteLater();
}

/**
 * @brief WickrIOClientLoginHdlr::loginNextUser
 * This method will try to login to the next WickrBot user on the list of WickrBot users
 */
void WickrIOClientLoginHdlr::loginNextUser()
{
    // If logged in then logout the current user
    if (m_loginState == LoggedIn) {
        m_loginState = LoggingOut;
        WickrCore::WickrSession::getActiveSession()->closeSession(true);

        // Not sure if have to wait for something here
        m_loginState = LoggedOut;
    }

    m_curLoginIndex++;

    if (m_curLoginIndex >= m_logins.size())
        m_curLoginIndex = 0;

    initiateLogin();
}
