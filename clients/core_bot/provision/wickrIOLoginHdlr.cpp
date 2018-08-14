#include "wickrIOLoginHdlr.h"

#include "filetransfer/wickrCloudTransferMgr.h"
#include "session/wickrRegistrationCtx.h"
#include "common/wickrRuntime.h"
#include "user/wickrUser.h"

#include "clientversioninfo.h"

WickrIOLoginHdlr::WickrIOLoginHdlr() :
    m_curLoginIndex(0),
    m_loginState(LoggedOut),
    m_consecutiveLoginFailures(0),
    m_backupVersion(-1),
    m_preRegDataIface(nullptr)
{
    m_firstLogin = ! WickrDBAdapter::doesDBExist();
}

WickrIOLoginHdlr::~WickrIOLoginHdlr()
{
    if (this->m_loginState == LoggedIn) {
    }
}

/**
 * @brief wickrMain::preRegistrationInit
 * Call this function to initialize, or cleanup the Pre-Registration data. A call to this
 * function will initialize or reset the user's signing key.
 */
void WickrIOLoginHdlr::preRegistrationInit()
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
QStringList WickrIOLoginHdlr::preRegistrationGetKeyStrings()
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
 * @brief WickrIOLoginHdlr::slotRegisterOnPrem
 */
void WickrIOLoginHdlr::slotRegisterOnPrem(const QString &username, const QString &password, const QString &newPassword, const QString &salt, const QString &transactionid, bool newUser, bool sync)
{
    QString hash = !salt.isEmpty() ? cryptoGetHash(password, salt) : QString();
    qDebug() << "Password Hash=" << hash;

    WickrEnterpriseRegistrationParameters regParams(username, transactionid, hash);
    WickrPreRegistrationData *preRegData;
    if (m_preRegDataIface != NULL) {
        preRegData = m_preRegDataIface->getPreRegData();
    } else {
        preRegData = NULL;
    }

    WickrRegisterUserContext *c = new WickrRegisterUserContext(username, newPassword, QString(), WickrUtil::getDeviceIdentifier(), WickrUtil::getDeviceHost(), newUser, sync, false, false, regParams, preRegData);
    c->putArg(arg_USERID,    username );
    c->putArg(arg_PASSWORD,  newPassword );
    if (!hash.isEmpty()) {
        c->putArg(arg_CHALLENGE, hash);
    }

    connect(c, &WickrRegisterUserContext::signalRequestCompleted,
            this, &WickrIOLoginHdlr::slotRegistrationDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief wickrMain::slotRegisterEnterprise
 * @param wickrid
 * @param password
 * @param transactionid
 * @param sync
 * @param isRekey
 */
void WickrIOLoginHdlr::slotRegisterUser(const QString &wickrid, const QString &password, const QString &salt, const QString &transactionid, bool newUser, bool sync, bool isRekey)
{
    qDebug().noquote().nospace() << "CONSOLE:Begin register user context.";

    QString hash = !salt.isEmpty() ? cryptoGetHash(password, salt) : QString();

    WickrEnterpriseRegistrationParameters regParams(wickrid, transactionid, hash);
    WickrPreRegistrationData *preRegData;
    if (m_preRegDataIface != NULL) {
        preRegData = m_preRegDataIface->getPreRegData();
    } else {
        preRegData = NULL;
    }

    WickrRegisterUserContext *c = new WickrRegisterUserContext(wickrid, password, QString(), WickrUtil::getDeviceIdentifier(), WickrUtil::getDeviceHost(), newUser, sync, isRekey, false, regParams, preRegData);
    c->putArg(arg_USERID,   wickrid );
    c->putArg(arg_PASSWORD, password );
    connect(c, &WickrRegisterUserContext::signalRequestCompleted,
            this, &WickrIOLoginHdlr::slotRegistrationDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOLoginHdlr::initiateLogin
 * This method will initiate a login using the credentials for one of the users from the
 * m_logins list variable. The index of the login to use is identified by the m_curLoginIndex
 * variable. The WickrRegistrationService will be called to initiate the login.
 * The database name will be set to the name of the user to be logged in.
 */
void WickrIOLoginHdlr::initiateLogin()
{
    if (m_consecutiveLoginFailures >= m_logins.size()) {
        m_loginState = LoginsFailed;
        emit signalLoginFailed();
    } else {
        m_loginState = InProcess;
        QString userid = m_logins.at(m_curLoginIndex)->m_name;
        QString password = m_logins.at(m_curLoginIndex)->m_pass;
        QString transid = m_logins.at(m_curLoginIndex)->m_transactionID;
        bool creatingUser = m_logins.at(m_curLoginIndex)->m_creating;

        WickrDBAdapter::setDBNameForUser( userid );

        if (creatingUser) {
            // If the database already exists then cannot do a Registration!
            if( WickrDBAdapter::doesDBExist() ) {
                qDebug() << "Creating user will fail because DB already exists!";
                emit signalLoginFailed();
            } else {
                qDebug() << "Starting registration to create user " << userid;
                QString salt;

                slotRegisterUser(userid, password, salt, transid, true, false, false);
            }
        } else {
            // If the database exists then just login
            if (WickrDBAdapter::doesDBExist()) {
                qDebug() << "Starting login for user " << userid;

                slotLoginStart(userid, password);
            } else {
                QString transID, salt;
                slotRegisterUser(userid, password, salt, transID, false, true, false);
            }
        }
    }
}

/**
 * @brief WickrIOLoginHdlr::slotRegistrationDone
 * This SLOT is called when the registration service is complete. A login request will be made to
 * login the user identified by the userid and password variables.
 * @param ws
 */
void WickrIOLoginHdlr::slotRegistrationDone(WickrRegisterUserContext *c)
{
    qDebug() << "Received registration";

    if(!c->isSuccess()) {
        // If we failed because of something other than bad credentials then show the result
        if (c->apiCode().getValue() == BAD_SYNC_CREDENTIALS) {
            qDebug().noquote().nospace() << "CONSOLE:" << c->errorString();
            emit signalLoginFailed();
        } else {
            qDebug().noquote().nospace() << "CONSOLE:" << c->errorString();
            emit signalOnlineFlag(false);
        }
    } else {
        // GET Arguments: <wickrid> <password>
        qDebug().noquote().nospace() << "CONSOLE:Successfully created user";
        QString wickrid  = c->getArg(arg_USERID).toString();
        QString password = c->getArg(arg_PASSWORD).toString();
        slotLoginStart( wickrid, password );
    }
    c->deleteLater();
}

void WickrIOLoginHdlr::slotLoginStart(const QString& username, const QString& password)
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
            QString salt;
            slotRegisterUser(user, pass, salt, unused, false, true, false);
        }
        return;
    }

    QString otp="";
    WickrLoginContext *c = new WickrLoginContext(username,password,otp,ClientVersionInfo::versionForLogin());
    connect(c, &WickrLoginContext::signalRequestCompleted, this, &WickrIOLoginHdlr::slotLoginDone, Qt::QueuedConnection);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

/**
 * @brief WickrIOLoginHdlr::slotLoginDone
 * This SLOT is called when the login service completes. If the login is successful then the
 * timer will be started so that processing can begin. If the login fails then the program will
 * be exited.
 * TODO: If the login fails we need to determine why. Maybe clear the database.
 *
 * @param ws Pointer to the login service object.
 */
void WickrIOLoginHdlr::slotLoginDone(WickrLoginContext *ls)
{
    if (ls->isSuccess()) {
        qDebug() << "Login successful";
        m_loginState = LoggedIn;

        // Need to display the Keys
        WickrCore::WickrUser *selfUser = WickrCore::WickrUser::getSelfUser();
        m_userSigningKey = selfUser->getUserSigningKey();

        emit signalLoginSuccess();

    } else {
        m_loginState = LoggedOut;

        // If this is an attempt to create the user then fail
        if (m_logins.at(m_curLoginIndex)->m_creating) {

            /* The login failed, should we reset the database?
             */
            qDebug() << "Login failed!";
            qDebug() << "Program exiting!";
            qDebug() << ls->errorString();


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
                qDebug() << "Login failed to server, but local login was success!";
                qDebug() << "Program exiting!";
                qDebug() << ls->errorString();

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

void WickrIOLoginHdlr::refreshDirectory()
{
    // Refresh the directory
    // NOTE: We will transition from RELOGIN_STARTED to RELOGIN_COMPLETE at the end of this
    //       retrieval of directory and profiles (via completeLogin())
    WickrDirectoryGetContext *c = new WickrDirectoryGetContext(0);
    connect(c, &WickrDirectoryGetContext::signalRequestCompleted,
            this, &WickrIOLoginHdlr::slotRefreshDirectoryDone);
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

// TODO: NEED TO GET THE BACKUP
void WickrIOLoginHdlr::slotRefreshDirectoryDone(WickrDirectoryGetContext* context)
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
 * @brief WickrIOLoginHdlr::loginNextUser
 * This method will try to login to the next WickrBot user on the list of WickrBot users
 */
void WickrIOLoginHdlr::loginNextUser()
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

