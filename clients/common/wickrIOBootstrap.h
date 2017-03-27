#ifndef WICKRIOBOOTSTRAP_H
#define WICKRIOBOOTSTRAP_H

#include <QObject>
#include <QVariant>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>
#include <QSslCertificate>

class EnvironmentMgr : public QObject
{
    Q_OBJECT

public:
    explicit EnvironmentMgr(QObject *parent = 0);

    Q_PROPERTY(bool isOnPrem READ hasConfiguration NOTIFY signalHasConfigurationChanged)

    Q_PROPERTY(bool canStartCall READ canStartCall WRITE setCanStartCall NOTIFY signalCanStartCallChanged)
    bool canStartCall() const;
    void setCanStartCall(const bool &canstartcall);

    Q_PROPERTY(bool friendFinder READ friendFinder WRITE setFriendFinder NOTIFY signalFriendFinderChanged)
    bool friendFinder() const;
    void setFriendFinder(const bool &friendfinder);

    Q_PROPERTY(QVariant forceDeviceLockout READ forceDeviceLockout WRITE setForceDeviceLockout NOTIFY signalForceDeviceLockout)
    QVariant forceDeviceLockout() const;
    void setForceDeviceLockout(const QVariant &forcedevicelockout);

    Q_PROPERTY(QVariant maxUploadSize READ maxUploadSize WRITE setMaxUploadSize NOTIFY signalMaxUploadSizeChanged)
    QVariant maxUploadSize() const;
    void setMaxUploadSize(const QVariant &maxuploadsize);

    Q_PROPERTY(QVariant maxAutoDownloadSize READ maxAutoDownloadSize WRITE setMaxAutoDownloadSize NOTIFY signalMaxAutoDownloadSizeChanged)
    QVariant maxAutoDownloadSize() const;
    void setMaxAutoDownloadSize(const QVariant &maxautodownloadsize);

    Q_PROPERTY(QVariantList availableEnvelopeTTL READ availableEnvelopeTTL WRITE setAvailableEnvelopeTTL NOTIFY onAvailableEnvelopeTTLChanged)
    QVariantList availableEnvelopeTTL() const;
    void setAvailableEnvelopeTTL(const QVariantList &availableenvelopettl);

    Q_PROPERTY(QVariantList availableEnvelopeBOR READ availableEnvelopeBOR WRITE setAvailableEnvelopeBOR NOTIFY onAvailableEnvelopeBORChanged)
    QVariantList availableEnvelopeBOR() const;
    void setAvailableEnvelopeBOR(const QVariantList &destructonreadenvelope);

    Q_PROPERTY(QVariant maxMessageTTL READ maxMessageTTL WRITE setMaxMessageTTL NOTIFY onAvailableEnvelopeTTLChanged)
    QVariant maxMessageTTL() const;
    void setMaxMessageTTL(const QVariant& ttl);

    Q_PROPERTY(QVariant maxMessageBOR READ maxMessageBOR WRITE setMaxMessageBOR NOTIFY onAvailableEnvelopeBORChanged)
    QVariant maxMessageBOR() const;
    void setMaxMessageBOR(const QVariant& bor);

    Q_PROPERTY(QVariant defaultMessageTTL READ defaultMessageTTL WRITE setDefaultMessageTTL NOTIFY onAvailableEnvelopeTTLChanged)
    QVariant defaultMessageTTL() const;
    void setDefaultMessageTTL(const QVariant& ttl);

    Q_PROPERTY(QVariant defaultMessageBOR READ defaultMessageBOR WRITE setDefaultMessageBOR NOTIFY onAvailableEnvelopeBORChanged)
    QVariant defaultMessageBOR() const;
    void setDefaultMessageBOR(const QVariant& bor);

    Q_PROPERTY(int maxMessageSize READ maxMessageSize)
    int maxMessageSize() const;

    Q_PROPERTY(int maxConvoMembers READ maxConvoMembers)
    int maxConvoMembers() const;

    Q_PROPERTY(bool requireEmailVerify READ requireEmailVerify WRITE setRequireEmailVerify)
    bool requireEmailVerify() const;
    void setRequireEmailVerify(const bool& value);

    Q_PROPERTY(bool requirePhoneVerify READ requirePhoneVerify WRITE setRequirePhoneVerify)
    bool requirePhoneVerify() const;
    void setRequirePhoneVerify(const bool& value);

    Q_PROPERTY(bool requireVideoVerify READ requireVideoVerify WRITE setRequireVideoVerify)
    bool requireVideoVerify() const;
    void setRequireVideoVerify(const bool& value);

    enum VerificationMode {
        VerificationRequired = 0,
        VerificationOptional = 1,
        VerificationNone = 2
    };
    Q_ENUM(VerificationMode)

    Q_PROPERTY(VerificationMode verificationMode READ verificationMode WRITE setVerificationMode NOTIFY verificationModeChanged)
    VerificationMode verificationMode() const;
    void setVerificationMode(VerificationMode verMode);

    Q_PROPERTY(bool canChangePassword READ canChangePassword WRITE setCanChangePassword NOTIFY onCanChangePasswordChanged)
    bool canChangePassword() const;
    void setCanChangePassword(const bool &canchangepassword);

    Q_PROPERTY(bool canResetPassword READ canResetPassword WRITE setCanResetPassword NOTIFY signalCanResetPasswordChanged)
    bool canResetPassword() const;
    void setCanResetPassword(const bool &canresetpassword);

    Q_PROPERTY(bool canCreateRoom READ canCreateRoom WRITE setCanCreateRoom NOTIFY signalCanCreateRoomChanged)
    bool canCreateRoom() const;
    void setCanCreateRoom(const bool &cancreateroom);

    Q_PROPERTY(bool canAddContact READ canAddContact WRITE setCanAddContact NOTIFY onCanAddContactChanged)
    bool canAddContact() const;
    void setCanAddContact(const bool &canaddcontact);

    Q_PROPERTY(bool alwaysReauthenticate READ alwaysReauthenticate WRITE setAlwaysReauthenticate NOTIFY signalAlwaysReauthenticateChanged)
    bool alwaysReauthenticate() const;
    void setAlwaysReauthenticate(const bool &alwaysreauthenticate);

    Q_PROPERTY(bool requireTransitiveTrust READ requireTransitiveTrust WRITE setRequireTransitiveTrust NOTIFY onRequireTransitiveTrustChanged)
    bool requireTransitiveTrust() const;
    void setRequireTransitiveTrust(const bool &requiretransitivetrust);

    Q_PROPERTY(QString userNameRules READ userNameRules WRITE setUserNameRules NOTIFY signalUserNameRulesChanged)
    QString userNameRules() const;
    void setUserNameRules(const QString &usernamerules);

    Q_PROPERTY(bool onlyShowInNetwork READ onlyShowInNetwork WRITE setOnlyShowInNetwork NOTIFY signalOnlyShowInNetworkChanged)
    bool onlyShowInNetwork() const;
    void setOnlyShowInNetwork(const bool &onlyshowinnetwork);

    Q_PROPERTY(bool emailAsUserIdMode READ emailAsUserIdMode WRITE setEmailAsUserIdMode NOTIFY signalEmailAsUserIdModeChanged)
    bool emailAsUserIdMode() const;
    void setEmailAsUserIdMode(const bool &emailasuseridmode);

    Q_PROPERTY(QString networkToken READ networkToken WRITE setNetworkToken NOTIFY signalNetworkTokenChanged)
    QString networkToken() const;
    void setNetworkToken(const QString &networktoken);

    bool loadJson(const QString & jsonStr, bool saveSettings = true);
    bool loadJson(const QJsonObject & jsonDoc, bool saveSettings = true);
    void updateEnvironment();
    void clearEnvironment();
    bool loadBootStrapJson(const QJsonDocument & jsonDoc);
    const QStringList & getCerts() { return m_certs; }
    const QStringList & getKeys() { return m_keys; }
    bool isTTLValid(long ttl);
    long calculateNewTTl(long ttl);
    bool isBORValid(long bor);
    long calculateNewBOR(long bor);
    void pin(QList<QSslCertificate> clist);

    // clear timestamp on logout so next login always reloads...
    // this way if reset to new account/network, will also work.
    void logoutReset(void) {
        m_lastUpdateTime = 0;
    }

    bool hasConfiguration() const {
        return m_hasConfiguration;
    }

    QList<QSslCertificate> pinned() const {
        return m_pinned;
    }

    bool isPinned(QList<QSslCertificate> peers);

    Q_INVOKABLE QString getRemainingStringFromTTL(const int ttl);

    void storeNetworkConfig(QSettings &settings);
    bool loadNetworkConfig(const QSettings &settings);

signals:
    // qml bridged signals
    void onAvailableEnvelopeTTLChanged();
    void onAvailableEnvelopeBORChanged();
    void onCanChangePasswordChanged();
    void onRequireTransitiveTrustChanged();
    void onCanAddContactChanged();
    void verificationModeChanged();

    // ordinary signals
    void signalHasConfigurationChanged();
    void signalCanStartCallChanged();
    void signalFriendFinderChanged();
    void signalForceDeviceLockout();
    void signalMaxUploadSizeChanged();
    //void signalMaxMessageTTLChanged();
    //void signalMaxMessageBORChanged();
    void signalMaxAutoDownloadSizeChanged();
    void signalCanResetPasswordChanged();
    void signalCanCreateRoomChanged();
    void signalAlwaysReauthenticateChanged();
    void signalUserNameRulesChanged();
    void signalOnlyShowInNetworkChanged();
    void signalEmailAsUserIdModeChanged();
    void signalAutoUpdateUrlChanged();
    void signalBugSnagUrlChanged();
    void signalNetworkTokenChanged();

private:
    bool m_hasConfiguration;
    ulong m_lastUpdateTime;
    bool m_canStartCall;
    bool m_friendFinder;
    QVariant m_forceDeviceLockout;
    QVariant m_defaultDeviceLockout;
    QVariant m_maxUploadSize;
    QVariant m_maxAutoDownloadSize;
    QVariant m_maxMessageTTL;
    QVariant m_maxMessageBOR;
    QVariant m_defaultMessageTTL;
    QVariant m_defaultMessageBOR;
    QVariantList m_availableEnvelopeTTL;
    bool m_requirePhoneVerify;
    bool m_requireEmailVerify;
    bool m_requireVideoVerify;
    bool m_canChangePassword;
    bool m_canResetPassword;
    bool m_canCreateRoom;
    bool m_canAddContact;
    bool m_alwaysReauthenticate;
    bool m_requireTransitiveTrust;
    QString m_userNameRules;
    bool m_onlyShowInNetwork;
    QVariantList m_availableEnvelopeBOR;
    bool m_emailAsUserIdMode;
    VerificationMode m_verificationMode;

    QJsonObject m_permissions;
    QString     m_networkToken;
    QStringList m_certs;
    QStringList m_keys;
    QList<QSslCertificate> m_pinned;
};

#endif // WICKRIOBOOTSTRAP_H
