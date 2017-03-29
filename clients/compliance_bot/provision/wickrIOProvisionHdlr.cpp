#include "common/wickrRuntime.h"
#include "common/wickrRequests.h"
#include "wickrIOProvisionHdlr.h"
#include "Wickr/WickrProduct.h"

#include <QProcess>

WickrIOProvisionHdlr::WickrIOProvisionHdlr(QObject *parent) : QObject(parent)
  , m_mode(CloudMode)
  , m_recordErrorMsg(tr("A technical error occurred.<br>Please re-record your video id."))
  , m_errorMsg(tr("A technical error occurred."))
  , m_validEmail(false)
  , m_verifiedEmail(false)
  , m_validCell(false)
  , m_verifiedCell(false)
  , m_invalidEmailDomain(false)
  , m_invalidSmsCode(false)
  , m_changePassword(false)
  , m_userExists(false)
  , m_currentStep(Step::init)
  , m_regStatus(RegistrationStatus::notYetTried)
  , m_networkLogo("")
  , m_networkAdmin("")
  , m_networkName("")
  , m_networkId("")
  , m_rejectedNetList(NULL)
  , indyJoined(false)
  , m_videoUploadComplete(false)
  , m_pages()
{
    switchToPage(Page::enterEmail);
}

void WickrIOProvisionHdlr::switchToPage(Page page)
{
    if (m_pages.isEmpty() || m_pages.last() != page) {
        if (m_pages.contains(page)) {
            while(m_pages.last() != page) {
                m_pages.removeLast();
            }
        } else {
            m_pages.append(page);
        }
        emit signalPageChanged(page);
    }
}

void WickrIOProvisionHdlr::startNextStep(WickrProvisionUserContext *ctx)
{
    if (ctx) {
        m_currentStep = (ctx->nextStep() == -1) ? Step::finished : (Step)ctx->nextStep();
    }

    switch(m_currentStep) {
    case Step::provisioningEmail:
        switchToPage(Page::enterEmail);
        break;

    case Step::checkingEmailVerificationStatus:
        switchToPage(Page::verifyEmail);
        checkEmail();
        break;

    case Step::provisioningCell:
        if (!m_cellphone.isEmpty()) {
            sendPhoneVerification();
        } else {
            switchToPage(Page::enterPhone);
        }
        break;

    case Step::verifyingCellCode:
        switchToPage(Page::verifyPhone);
        break;

    case Step::checkingCellVerificationStatus:    // ignored
        dummyStep();
        break;

    case Step::forgotPasswordVerifyEmail:
        qDebug() << "Got forgotPasswordVerificationStatus from server. That shouldn't happen.";
        break;

    case Step::checkingEmailVerificationStatus2:
        switchToPage(Page::verifyEmail);
        checkEmail();
        break;

    case Step::uploadingVerificationVideo:
        switchToPage(Page::askVideoPermission);
        break;

    case Step::finished:
        if (m_mode == OnPremMode) {
            if (m_changePassword) {
//                registerWithPassword(m_password);
                switchToPage(Page::enterPassword);
            } else {
                registerWithPassword(m_password);
            }
        } else if (m_mode == CloudMode) {
            switchToPage(Page::enterPassword);
        } else if (m_mode == ForgotPasswordMode) {
            switchToPage(Page::enterPassword);
        }
        break;

    default:
        qDebug() << "===== CASE" << m_currentStep << "WASN'T HANDLED!!!! =====";
        return;
    }
}

void WickrIOProvisionHdlr::dummyStep()
{
    QMap<QString,QVariant> map;
    map.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    map.insert(WickrProvisionUserContext::USER_TRANSACTIONID, m_transID);

    int currentStep = m_currentStep;

    WickrProvisionUserContext *provUserSvc = new WickrProvisionUserContext(map);
    connect(provUserSvc, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if (c->isSuccess()) {
            qDebug() << c->reqAsString() << ": Step" << currentStep << "succeeded!";

            m_transID = c->get(WickrProvisionUserContext::USER_TRANSACTIONID).toString();
            qDebug() << c->reqAsString() << ": Transaction_id=" << m_transID;

            startNextStep(c);
        } else {
            qDebug() << c->reqAsString() << ": Step" << currentStep << "failed!";
            qDebug() << c->reqAsString() << ": getResult() = " << c->errorString();

            // TODO: DISPLAY ERROR TO USER
            qDebug() << "ERROR:" << m_errorMsg;
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(provUserSvc);
}

void WickrIOProvisionHdlr::onPremBegin(const QString username, const QString password, const QString regToken)
{
    Q_ASSERT(!username.isEmpty() && !password.isEmpty() && !regToken.isEmpty());
    internalBegin(OnPremMode, QString(), QString(), username.trimmed(), password.trimmed(), regToken.trimmed());
}

void WickrIOProvisionHdlr::cloudBegin(const QString &email, const QString &inviteCode)
{
    if (!email.isEmpty() && !inviteCode.isEmpty()) {
        internalBegin(CloudMode, email.trimmed(), inviteCode.trimmed(), QString(), QString(), QString());
    }
}

void WickrIOProvisionHdlr::forgotPasswordBegin(const QString &email)
{
    if(!email.isEmpty()) {
        internalBegin(ForgotPasswordMode, email.trimmed(), QString(), QString(), QString(), QString());
    }
}

bool WickrIOProvisionHdlr::provisionBotUser(Mode mode)
{
    QMap<QString,QVariant> map;


    if (mode == OnPremMode) {
        if (m_username.isEmpty() || m_regToken.isEmpty()) {
            return false;
        }
        map.insert(WickrProvisionUserContext::USER_STEP, 0);
        map.insert(WickrProvisionUserContext::USER_USERNAME, m_username);
        map.insert(WickrProvisionUserContext::USER_REGTOKEN, m_regToken);
    } else {
        if (m_invitecode.isEmpty() || m_email.isEmpty()) {
            return false;
        }
        map.insert(WickrProvisionUserContext::USER_STEP, 1);
        map.insert(WickrProvisionUserContext::USER_PRODUCT, wickrProductGetProductType());
        map.insert(WickrProvisionUserContext::USER_EMAIL, m_email);
        map.insert(WickrProvisionUserContext::USER_ICODE, m_invitecode);
    }

    WickrProvisionUserContext *provUserSvc = new WickrProvisionUserContext(map);
    connect(provUserSvc, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if (c->isSuccess()) {
            qDebug() << c->reqAsString() << ": internalBegin: succeeded!";

            m_transID = c->get(WickrProvisionUserContext::USER_TRANSACTIONID).toString();
            qDebug() << c->reqAsString() << ": Transaction_id=" << m_transID;

            m_passwordSalt = c->get(WickrProvisionUserContext::USER_SALT).toString();
            qDebug() << c->reqAsString() << ": salt (password SALT) =" << m_passwordSalt;

            bool ok;
            int changePassword = c->get(WickrProvisionUserContext::USER_CHANGEPASSWORD).toInt(&ok);
            if (ok && changePassword) {
                m_changePassword = true;
            }

            registerWithPassword(m_password);
        } else {
            qDebug() << c->reqAsString() << ": internalBegin: failed!";
            qDebug() << c->reqAsString() << ": getResult() = " << c->errorString();

            if (c->invalidEmailDomain()) {
                setInvalidEmail(true);
            } else if (c->userAlreadyExists()) {
                m_userExists = true;
                registerWithPassword(m_password);
            } else {
                // Show error
                qDebug().noquote().nospace() << c->reqAsString() << ": internalBegin: FAILED! " << c->errorString();

                // TODO: DISPLAY ERROR TO USER
                qDebug() << "ERROR:" << m_errorMsg;
            }
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(provUserSvc);
    return true;
}

void WickrIOProvisionHdlr::internalBegin(Mode mode, const QString &email, const QString &inviteCode, const QString &username, const QString &password, const QString &regToken)
{
    m_mode = mode;

    m_email = email;
    m_invitecode = inviteCode;
    m_username = username;
    m_password = password;
    m_regToken = regToken;

    emit emailChanged();
    emit usernameChanged();
    emit invitecodeChanged();
    emit regTokenChanged();
    emit passwordChanged();

    if (m_mode == OnPremMode) {
        m_currentStep = provisioningUserName;
    } else {
        m_currentStep = (m_mode == ForgotPasswordMode) ? Step::forgotPasswordVerifyEmail : Step::provisioningEmail;
    }

    QMap<QString,QVariant> map;

    map.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    map.insert(WickrProvisionUserContext::USER_PRODUCT, wickrProductGetProductType());

    if (!m_email.isEmpty())       map.insert(WickrProvisionUserContext::USER_EMAIL, m_email);
    if (!m_username.isEmpty())    map.insert(WickrProvisionUserContext::USER_USERNAME, m_username);
    if (!m_regToken.isEmpty())    map.insert(WickrProvisionUserContext::USER_REGTOKEN, m_regToken);
    if (!m_invitecode.isEmpty())  map.insert(WickrProvisionUserContext::USER_ICODE, m_invitecode);

    setVerifiedEmail(false);

    WickrProvisionUserContext *provUserSvc = new WickrProvisionUserContext(map);
    connect(provUserSvc, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if (c->isSuccess()) {
            qDebug() << c->reqAsString() << ": internalBegin: succeeded!";

            m_transID = c->get(WickrProvisionUserContext::USER_TRANSACTIONID).toString();
            qDebug() << c->reqAsString() << ": Transaction_id=" << m_transID;

            m_passwordSalt = c->get(WickrProvisionUserContext::USER_SALT).toString();
            qDebug() << c->reqAsString() << ": salt (password SALT) =" << m_passwordSalt;

            if (m_passwordSalt.isEmpty() && m_mode == OnPremMode) {
                qDebug() << "New registration failed to receive SALT value";
                setRegStatus(WickrIOProvisionHdlr::RegistrationStatus::failed);
                c->deleteLater();
                return;
            }

            m_cellphone = c->get(WickrProvisionUserContext::USER_CELL).toString();
            qDebug() << c->reqAsString() << ": cell =" << m_cellphone;

            m_invalidEmailDomain = false;
            emit invalidEmailDomainChanged();

            bool ok;
            int changePassword = c->get(WickrProvisionUserContext::USER_CHANGEPASSWORD).toInt(&ok);
            if (ok && changePassword) {
                m_changePassword = true;
            }

            startNextStep(c);
        } else {
            qDebug() << c->reqAsString() << ": internalBegin: failed!";
            qDebug() << c->reqAsString() << ": getResult() = " << c->errorString();

            if (c->invalidEmailDomain()) {
                setInvalidEmail(true);
            } else if (c->userAlreadyExists()) {
                m_userExists = true;
                registerWithPassword(m_password);
            } else {
                // Show error
                qDebug().noquote().nospace() << c->reqAsString() << ": internalBegin: FAILED! " << c->errorString();

                // TODO: DISPLAY ERROR TO USER
                qDebug() << "ERROR:" << m_errorMsg;
            }
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(provUserSvc);
}

void WickrIOProvisionHdlr::sendPhoneVerification()
{
    if(m_transID.isNull() || m_cellphone.isEmpty())
    {
        return;
    }

    // m_transID = provUserSvc->getRequestMap().value(WickrProvisionUserContext::USER_TRANSACTIONID);
    QMap<QString,QVariant> step3map;
    m_currentStep = Step::provisioningCell;

    step3map.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    step3map.insert(WickrProvisionUserContext::USER_TRANSACTIONID, m_transID);
    step3map.insert(WickrProvisionUserContext::USER_CELL, m_cellphone);
    qDebug() << "sending code to cell phone " << m_cellphone;
    WickrProvisionUserContext *c = new WickrProvisionUserContext(step3map);
    connect(c, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if (c->isSuccess()) {
            qDebug().noquote().nospace() << c->reqAsString() << ": sendPhoneVerification: Success";

            m_transID = c->get(WickrProvisionUserContext::USER_TRANSACTIONID).toString();
            qDebug().noquote().nospace() << c->reqAsString() << ": Transaction_id=" << m_transID;

            startNextStep(c);
        } else {
            qDebug().noquote().nospace() << c->reqAsString() << ": sendPhoneVerification: FAILED!";
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);

    // As an optimization, we immediately switch to the verifyPhone page without waiting for the server response.
    // It's possible that we might then jump to a different page when the server responds.
    switchToPage(Page::verifyPhone);
}

void WickrIOProvisionHdlr::testEnteredSMSCode()
{
    if(m_transID.isNull() || m_verificationCodeCell.isEmpty())
    {
        return;
    }

    QMap<QString,QVariant> argumentMap;
    m_currentStep = Step::verifyingCellCode;

    argumentMap.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    argumentMap.insert(WickrProvisionUserContext::USER_TRANSACTIONID, m_transID);
    // Need a way to get this value here!
    argumentMap.insert(WickrProvisionUserContext::USER_CELLAUTH, m_verificationCodeCell);

    WickrProvisionUserContext *c = new WickrProvisionUserContext(argumentMap);

    connect(c, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if (c->isSuccess()) {
            qDebug().noquote().nospace() << c->reqAsString() << ": testEnteredSMSCode: Success";

            m_transID = c->get(WickrProvisionUserContext::USER_TRANSACTIONID).toString();
            qDebug().noquote().nospace() << c->reqAsString() << ": Transaction_id=" << m_transID;

            m_invalidSmsCode = false;
            emit invalidSmsCodeChanged();

            startNextStep(c);
        } else {
            qDebug().noquote().nospace() << c->reqAsString() << ": testEnteredSMSCode: FAILED!";
            m_invalidSmsCode = true;
            emit invalidSmsCodeChanged();
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

void WickrIOProvisionHdlr::checkEmail()
{
    QMap<QString,QVariant> argumentMap;
    argumentMap.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    argumentMap.insert(WickrProvisionUserContext::USER_TRANSACTIONID, m_transID);

    WickrProvisionUserContext *c = new WickrProvisionUserContext(argumentMap);

    checkVerifiedEmailTmr.setInterval(5000);
    connect(&checkVerifiedEmailTmr, &QTimer::timeout, this, &WickrIOProvisionHdlr::checkEmail);  // TODO: If there's a previous request, shouldn't we disconnect it?

    connect(c, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        if(m_transID.isNull()) {
            return;
        }

        if (c->isSuccess()) {
            qDebug().noquote().nospace() << c->reqAsString() << ": checkEmail: Success";
            QVariant variant = c->get(WickrProvisionUserContext::USER_EMAIL);

            int result = variant.toInt();
            if (result) {
                checkVerifiedEmailTmr.stop();
                qDebug().noquote().nospace() << c->reqAsString() << ": EMAIL is Verified!";
                setVerifiedEmail(true);
                startNextStep(c);
            } else {
                if(!checkVerifiedEmailTmr.isActive()) {
                    checkVerifiedEmailTmr.start();
                }
                qDebug().noquote().nospace() << c->reqAsString() << ": " << result;
                qDebug().noquote().nospace() << c->reqAsString() << ": EMAIL NOT Verified!  Retrying!";
            }

        } else {
            qDebug().noquote().nospace() << c->reqAsString() << ": checkEmail: FAILED! " << c->errorString();
            // TODO: DISPLAY ERROR TO USER
            qDebug() << "ERROR:" << m_errorMsg;
        }
        c->deleteLater();
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(c);
}

void WickrIOProvisionHdlr::registerWithPassword(const QString &newPassword)
{
    if (!newPassword.isEmpty() && (!m_email.isEmpty() || !m_username.isEmpty())) {
        // TODO: OLD CODE WAS DOING A RESET
        if (m_mode == OnPremMode) {
            m_currentStep = Step::finished;  // If anyone asks, we are done!
            emit signalRegisterOnPrem(m_username, m_password, newPassword, !m_userExists ? m_passwordSalt : QString(), m_transID, !m_userExists, m_userExists);
        } else if (m_mode == CloudMode || m_mode == ForgotPasswordMode) {
            m_currentStep = Step::loggingIn;
            emit signalRegisterEnterprise(m_email, newPassword, m_transID, true, false, (m_mode == ForgotPasswordMode));  // Begins a process that eventually calls WickrIOProvisionHdlr::loginComplete()
        }
    }
}

void WickrIOProvisionHdlr::uploadVideo()
{
    QFile file(m_videoFilename);
    QByteArray blob = file.readAll();
    file.close();
    QMap<QString,QVariant> map;
    m_currentStep = Step::uploadingVerificationVideo;
    map.insert(WickrProvisionUserContext::USER_STEP, m_currentStep);
    map.insert(WickrProvisionUserContext::USER_TRANSACTIONID, m_transID);

    WickrProvisionUserContext *provUser = new WickrProvisionUserContext(map, blob);
    connect(provUser, &WickrProvisionUserContext::signalRequestCompleted, this, [=](WickrProvisionUserContext *c) {
        c->deleteLater();

        qDebug() << "video upload finished";
        m_videoUploadComplete = true;

        // the video upload is intended to occur in the background while the user is entering their password.
        // there are two situations that may happen
        // 1. IF the video uploaded before the user entered a password THEN let the register button continue
        // 2. IF the video is still uploading and the user enters a password THEN block the registration until the upload
        //    is completed.  So if in this done routine, the m_passwordAccepted is TRUE then the user clicked "Register"
        //    before the video completed so now it needs to be called again with the m_videoUploadComplete flag now true

        startNextStep(c);
    });
    WickrCore::WickrRuntime::taskSvcMakeRequest(provUser);
    qDebug() << "starting video upload";
}

void WickrIOProvisionHdlr::processComplete()
{
    m_currentStep = Step::finished;
    // fetching the directory will transition to the main screen
}

void WickrIOProvisionHdlr::loginComplete()
{
    Q_ASSERT(m_currentStep == Step::loggingIn);
    Q_ASSERT(m_mode == CloudMode);
}

void WickrIOProvisionHdlr::setInvalidEmail(bool bInvalidEmail)
{
    if (m_invalidEmailDomain != bInvalidEmail) {
        m_invalidEmailDomain  = bInvalidEmail;
        emit invalidEmailDomainChanged();
    }
}
