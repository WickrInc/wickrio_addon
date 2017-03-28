#ifndef WICKRIOPROVISIONHDLR_H
#define WICKRIOPROVISIONHDLR_H

#include <QObject>
#include <QTimer>
#include "common/wickrNetworkUtil.h"
#include "common/wickrRequests.h"

class WickrIOProvisionHdlr : public QObject
{
    Q_OBJECT

    enum Step {
        init = 0,

        // The first 8 steps have values that are used to communicate with the server. Don't change them.
        // Also, try not to make assumptions about what order these will occur in.
        provisioningEmail =                   1,
        checkingEmailVerificationStatus =     2,
        provisioningCell =                    3,
        verifyingCellCode =                   4,
        checkingCellVerificationStatus =      5,
        forgotPasswordVerifyEmail =           6,
        checkingEmailVerificationStatus2 =    7,
        uploadingVerificationVideo =          8,

        loggingIn = 20,

        askingForContactsPermission,
        networkLooking,
        networkFound,

        finished
    };

    enum {
        LastServerStep = uploadingVerificationVideo
    };

    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(bool invalidEmailDomain READ invalidEmailDomain NOTIFY invalidEmailDomainChanged)
    Q_PROPERTY(bool invalidSmsCode READ invalidSmsCode NOTIFY invalidSmsCodeChanged)

    Q_PROPERTY(QString invitecode READ invitecode WRITE setInvitecode NOTIFY invitecodeChanged)
    Q_PROPERTY(QString regToken READ regToken WRITE setRegToken NOTIFY regTokenChanged)
    Q_PROPERTY(QString cellphone READ cellphone WRITE setCellphone NOTIFY cellphoneChanged)
    Q_PROPERTY(QString flagImg READ flagImg WRITE setFlagImg NOTIFY flagImgChanged)
    Q_PROPERTY(QString verificationCodeEmail READ verificationCodeEmail WRITE setVerificationCodeEmail NOTIFY verificationCodeEmailChanged)
    Q_PROPERTY(QString verificationCodeCell READ verificationCodeCell WRITE setVerificationCodeCell NOTIFY verificationCodeCellChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

    Q_PROPERTY(int page READ page NOTIFY signalPageChanged)
    Q_PROPERTY(int regStatus READ regStatus NOTIFY regStatusChanged)


    Q_PROPERTY(bool validEmail READ validEmail WRITE setValidEmail NOTIFY validEmailChanged)
    Q_PROPERTY(bool verifiedEmail READ verifiedEmail NOTIFY verifiedEmailChanged)

    Q_PROPERTY(bool validCell READ validCell WRITE setValidCell NOTIFY validCellChanged)
    Q_PROPERTY(bool verifiedCell READ verifiedCell NOTIFY verifiedCellChanged)

    Q_PROPERTY(QString networkLogo READ networkLogo WRITE setNetworkLogo NOTIFY networkLogoChanged)
    Q_PROPERTY(QString networkAdminEmail READ networkAdminEmail WRITE setNetworkAdminEmail NOTIFY networkAdminEmailChanged)
    Q_PROPERTY(QString networkName READ networkName WRITE setNetworkName NOTIFY networkNameChanged)


public:
    enum Page {
        enterEmail,
        verifyEmail,
        enterPhone,
        verifyPhone,
        askVideoPermission,
        recordVideo,
        enterPassword,  // The log in screen
        askContactsPermission,
        askJoinNetwork,
    };

    enum RegistrationStatus
    {
        notYetTried = 0,
        success = 1,
        failed = 2
    };

    explicit WickrIOProvisionHdlr(QObject *parent = 0);

    Q_INVOKABLE bool inRegistrationProcess() const
    {
        return m_mode != ForgotPasswordMode && m_currentStep == loggingIn;
    }

    void setEmail(const QString& email)
    {
        if(m_email != email)
        {
            m_email = email;
            emit emailChanged();
        }
    }

    void setUsername(const QString& username)
    {
        if(m_username != username)
        {
            m_username = username;
            emit usernameChanged();
        }
    }

    void setInvitecode(const QString& invitecode)
    {
        if(m_invitecode != invitecode)
        {
            m_invitecode = invitecode;
            emit invitecodeChanged();
        }
    }

    void setRegToken(const QString& regToken)
    {
        if(m_regToken != regToken)
        {
            m_regToken = regToken;
            emit regTokenChanged();
        }
    }

    void setCellphone(const QString& cellphone)
    {
        if(m_cellphone != cellphone)
        {
            m_cellphone = cellphone;
            emit cellphoneChanged();
        }
    }

    void setFlagImg(const QString& flagImg)
    {
        if(m_flagImg != flagImg)
        {
            m_flagImg = flagImg;
            emit flagImgChanged();
        }
    }

    void setVerificationCodeEmail(const QString& verificationCodeEmail)
    {
        if(m_verificationCodeEmail != verificationCodeEmail)
        {
            m_verificationCodeEmail = verificationCodeEmail;
            emit verificationCodeEmailChanged();
        }
    }


    void setVerificationCodeCell(const QString& verificationCodeCell)
    {
        if(m_verificationCodeCell != verificationCodeCell)
        {
            m_verificationCodeCell = verificationCodeCell;
            emit verificationCodeCellChanged();
        }
    }


    void setValidEmail(bool validEmail)
    {
        if(m_validEmail != validEmail)
        {
            m_validEmail = validEmail;
            emit validEmailChanged();
        }
    }

    void setVerifiedEmail(bool verifiedEmail)
    {
        if(m_verifiedEmail != verifiedEmail)
        {
            m_verifiedEmail = verifiedEmail;
            emit verifiedEmailChanged();
        }
    }

    void setValidCell(bool validCell)
    {
        if(m_validCell != validCell)
        {
            m_validCell = validCell;
            emit validCellChanged();
        }
    }

    void setVerifiedCell(bool verifiedCell)
    {
        if(m_verifiedCell!= verifiedCell)
        {
            m_verifiedCell = verifiedCell;
            emit verifiedCellChanged();
        }
    }

    void setPassword(const QString& password)
    {
        if(m_password!= password)
        {
            m_password = password;
            emit passwordChanged();
        }
    }

    void setRegStatus(const RegistrationStatus status)
    {
        if(m_regStatus!= status)
        {
            m_regStatus = status;
            emit regStatusChanged();
        }
    }

    void setNetworkLogo(const QString logo)
    {
        if(m_networkLogo!= logo)
        {
            m_networkLogo = logo;
            emit networkLogoChanged();
            qDebug() << logo;
        }
    }

    void setNetworkAdminEmail(const QString admin)
    {
        m_networkAdmin = admin;
        emit networkAdminEmailChanged();
    }

    void setNetworkName(const QString name)
    {
        m_networkName = name;
        emit networkNameChanged();
    }

    void setNetworkId(const QString id)
    {
        m_networkId = id;
    }

    void setRejectedNetworkIds(const QStringList list)
    {
        m_rejectedNetList = list;
    }

    const QString& email() const { return m_email; }
    const QString& username() const { return m_username; }
    const QString& transactionID() const { return m_transID; }

    bool invalidEmailDomain() const { return m_invalidEmailDomain; }
    bool invalidSmsCode() const { return m_invalidSmsCode; }
    const QString& invitecode() const { return m_invitecode; }
    const QString& regToken() const { return m_regToken; }
    const QString& cellphone() const { return m_cellphone; }
    const QString& flagImg() const { return m_flagImg; }
    const QString& verificationCodeEmail() const { return m_verificationCodeEmail; }
    const QString& verificationCodeCell() const { return m_verificationCodeCell; }
    int page() const { return m_pages.size() ? m_pages.last() : -1; }
    const QString& password() const { return m_password; }
    bool validEmail() const { return m_validEmail; }
    bool verifiedEmail() const { return m_verifiedEmail; }
    bool validCell() const { return m_validCell; }
    bool verifiedCell() const { return m_verifiedCell; }
    int regStatus() const { return m_regStatus; }
    const QString& networkLogo() const { return m_networkLogo; }
    const QString& networkAdminEmail() const { return m_networkAdmin; }
    const QString& networkName() const { return m_networkName; }
    const QString& networkId() const { return m_networkId; }
    const QStringList& rejectedNetworkIds() const { return m_rejectedNetList; }

    void startNextStep(WickrProvisionUserContext *ctx);

    Q_INVOKABLE void onPremBegin(const QString username, const QString password, const QString regToken);
    Q_INVOKABLE void cloudBegin(const QString &email, const QString &inviteCode);
    Q_INVOKABLE void forgotPasswordBegin(const QString &email);

    Q_INVOKABLE void sendPhoneVerification();
    Q_INVOKABLE void testEnteredSMSCode();
    Q_INVOKABLE void registerWithPassword(const QString &password);
    Q_INVOKABLE void uploadVideo();
    Q_INVOKABLE void setInvalidEmail(bool bInvalidEmail);

    void loginComplete();

signals:
    void signalPageChanged(Page);

    void emailChanged();
    void usernameChanged();
    void invalidEmailDomainChanged();
    void invitecodeChanged();
    void regTokenChanged();
    void cellphoneChanged();
    void flagImgChanged();
    void verificationCodeEmailChanged();
    void verificationCodeCellChanged();
    void validEmailChanged();
    void verifiedEmailChanged();
    void validCellChanged();
    void verifiedCellChanged();
    void passwordChanged();
    void processFinished();
    void regStatusChanged();
    void invalidSmsCodeChanged();
    void networkLogoChanged();
    void networkAdminEmailChanged();
    void networkNameChanged();
    void clearAll();

    void signalRegisterOnPrem(const QString &username, const QString &password, const QString &newPassword, const QString &salt, const QString &transactionid, bool newUser, bool sync);
    void signalRegisterEnterprise(const QString &userId, const QString &password, const QString &transactionid, bool newUser, bool sync, bool isRekey);


public slots:
    void checkEmail();

private:
    enum Mode {
        CloudMode,
        OnPremMode,
        ForgotPasswordMode
    } m_mode;

    const QString m_recordErrorMsg;
    const QString m_errorMsg;

    QString m_email;
    QString m_username;
    QString m_invitecode;
    QString m_regToken;
    QString m_cellphone;
    QString m_flagImg;
    QString m_verificationCodeEmail;
    QString m_verificationCodeCell;
    QString m_password;
    bool m_validEmail;
    bool m_verifiedEmail;
    bool m_validCell;
    bool m_verifiedCell;
    bool m_invalidEmailDomain;
    bool m_invalidSmsCode;
    bool m_changePassword;
    bool m_userExists;
    Step m_currentStep;
    QString m_videoFilename;

    RegistrationStatus m_regStatus;

    QString m_networkLogo;
    QString m_networkAdmin;
    QString m_networkName;
    QString m_networkId;
    QStringList m_rejectedNetList;
    QList<WickrClientNetworkInvite *> m_invites;

    QString m_transID;
    QString m_passwordSalt;
    QTimer checkVerifiedEmailTmr;

    QImage networkImage;

    void processComplete();
    bool indyJoined;
    bool m_videoUploadComplete;

    QList<Page> m_pages;
    void switchToPage(Page page);

    void internalBegin(Mode mode, const QString &email, const QString &inviteCode, const QString &username, const QString &password, const QString &regToken);
    void dummyStep();

};

#endif // WICKRIOPROVISIONHDLR_H
