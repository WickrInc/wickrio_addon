#include "wickrIOBootstrap.h"
#include "session/wickrSession.h"
#include "common/wickrUtil.h"
#include <QSslCertificate>
#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QCryptographicHash>
#include <QSslKey>

static const QString ENV_PERMMOD = "permmod";
static const QString ENV_STARTCALL = "canStartCall";
static const QString ENV_FRIENDFINDER = "friendFinder";
static const QString ENV_DEVICELOCKOUT = "forceDeviceLockout";
static const QString ENV_MAXDOWNLOAD = "maxAutoDownloadSize";
static const QString ENV_MAXUPLOAD = "maxUploadSize";
static const QString ENV_AVAILTTL = "availableEnvelopeTTL";
static const QString ENV_AVAILBOR = "destructOnRead";
static const QString ENV_CHANGEPASSWORD = "canChangePassword";
static const QString ENV_RESETPASSWORD = "canResetPassword";
static const QString ENV_TRANSITIVETRUST = "requireTransitiveTrust";
static const QString ENV_REAUTHENTICATE = "alwaysReauthenticate";
static const QString ENV_NAMERULES = "userNameRules";
static const QString ENV_ADDCONTACT = "canAddContact";
static const QString ENV_ONLYSHOWINNETWORK = "onlyShowInNetwork";
static const QString ENV_CREATEROOM = "canCreateRoom";
static const QString ENV_MAXTTL = "maxTTL";
static const QString ENV_MAXBOR = "maxBOR";
static const QString ENV_VERIFICATIONMODE = "verificationMode";

EnvironmentMgr::EnvironmentMgr(QObject *parent)
    : QObject(parent)
    , m_hasConfiguration(false)
    , m_lastUpdateTime(0)
{
    clearEnvironment();
}

/**
 * @brief EnvironmentMgr::canStartCall
 * @return
 */
bool EnvironmentMgr::canStartCall() const
{
#ifndef WICKR_NPL_ENABLED
    return false;
#else
    return m_canStartCall;
#endif
}

/**
 * @brief EnvironmentMgr::setCanStartCall
 * @param canstartcall
 */
void EnvironmentMgr::setCanStartCall(const bool &canstartcall)
{
    if(canstartcall != m_canStartCall)
    {
        m_canStartCall = canstartcall;
#ifdef WICKR_NPL_ENABLED
        emit signalCanStartCallChanged();
#endif
    }
}

/**
 * @brief EnvironmentMgr::friendFinder
 * @return
 */
bool EnvironmentMgr::friendFinder() const
{
    return m_friendFinder;
}

/**
 * @brief EnvironmentMgr::setFriendFinder
 * @param friendfinder
 */
void EnvironmentMgr::setFriendFinder(const bool &friendfinder)
{
    if(friendfinder != m_friendFinder)
    {
        m_friendFinder = friendfinder;
        emit signalFriendFinderChanged();
    }
}

/**
 * @brief EnvironmentMgr::forceDeviceLockout
 * @return
 */
QVariant EnvironmentMgr::forceDeviceLockout() const
{
    if (m_forceDeviceLockout.isNull())
        return 0;

    return m_forceDeviceLockout;
}

/**
 * @brief EnvironmentMgr::setForceDeviceLockout
 * @param forcedevicelockout
 */
void EnvironmentMgr::setForceDeviceLockout(const QVariant &forcedevicelockout)
{
    QVariant lockout = m_defaultDeviceLockout;
    if(forcedevicelockout.toInt() >= 0)
        lockout = forcedevicelockout;

    if(lockout != m_forceDeviceLockout) {
        m_forceDeviceLockout = lockout;
        emit signalForceDeviceLockout();
    }
}

/**
 * @brief EnvironmentMgr::maxUploadSize
 * @return
 */
QVariant EnvironmentMgr::maxUploadSize() const
{
    return m_maxUploadSize;
}

/**
 * @brief EnvironmentMgr::setMaxUploadSize
 * @param maxuploadsize
 */
void EnvironmentMgr::setMaxUploadSize(const QVariant &maxuploadsize)
{
    if(maxuploadsize != m_maxUploadSize)
    {
        m_maxUploadSize = maxuploadsize;

#ifdef Q_OS_WIN
        // basically give windows users a little upload bump so it appears
        // to be correct (5 MB = 5,000,000 bytes)
        if(!m_maxUploadSize.isNull())
        {
            double num = m_maxUploadSize.toDouble();
            if(num > 1000000000000)
            {
                num  = num * 1024.0 / 1000.0;
            }
            if(num > 1000000000)
            {
                num  = num * 1024.0 / 1000.0;
            }
            if(num > 1000000)
            {
                num  = num * 1024.0 / 1000.0;
            }
            if(num > 1000)
            {
                num  = num * 1024.0 / 1000.0;
            }
            m_maxUploadSize = (int) num;
        }
#endif
        emit signalMaxUploadSizeChanged();
    }
}

QVariant EnvironmentMgr::maxMessageTTL() const
{
    return m_maxMessageTTL;
}

QVariant EnvironmentMgr::defaultMessageTTL() const
{
    return m_defaultMessageTTL;
}

QVariant EnvironmentMgr::defaultMessageBOR() const
{
    return m_defaultMessageBOR;
}

void EnvironmentMgr::setMaxMessageTTL(const QVariant& ttl)
{
    if(ttl != m_maxMessageTTL) {
        m_maxMessageTTL = ttl;

#ifdef WICKR_SCIF
        if(m_maxMessageTTL > (86400 * 6))
            m_maxMessageTTL = 86400 * 6;
#endif

        emit onAvailableEnvelopeTTLChanged();
//        emit signalMaxMessageTTLChanged();
    }
}

void EnvironmentMgr::setDefaultMessageTTL(const QVariant& ttl)
{
    if(ttl != m_defaultMessageTTL) {
        m_defaultMessageTTL = ttl;

#ifdef WICKR_SCIF
        if(m_defaultMessageTTL > (86400 * 6))
            m_defaultMessageTTL = 86400 * 6;
#endif
        emit onAvailableEnvelopeTTLChanged();
//        emit signalMaxMessageTTLChanged();
    }
}

QVariant EnvironmentMgr::maxMessageBOR() const
{
    return m_maxMessageBOR;
}

void EnvironmentMgr::setMaxMessageBOR(const QVariant& bor)
{
    if(bor != m_maxMessageBOR) {
        m_maxMessageBOR = bor;

#ifdef WICKR_SCIF
        if(m_maxMessageBOR > (86400 * 6))
            m_maxMessageBOR = 86400 * 6;
#endif

        emit onAvailableEnvelopeBORChanged();
//        emit signalMaxMessageBORChanged();
    }
}

void EnvironmentMgr::setDefaultMessageBOR(const QVariant& bor)
{
    if(bor != m_defaultMessageBOR) {
        m_defaultMessageBOR = bor;

#ifdef WICKR_SCIF
        if(m_defaultMessageBOR > (86400 * 6))
            m_defaultMessageBOR = 86400 * 6;
#endif

        emit onAvailableEnvelopeBORChanged();
//        emit signalMaxMessageBORChanged();
    }
}

/**
 * @brief EnvironmentMgr::maxAutoDownloadSize
 * @return
 */
QVariant EnvironmentMgr::maxAutoDownloadSize() const
{
    return m_maxAutoDownloadSize;
}

/**
 * @brief EnvironmentMgr::setMaxAutoDownloadSize
 * @param maxautodownloadsize
 */
void EnvironmentMgr::setMaxAutoDownloadSize(const QVariant &maxautodownloadsize)
{
    if(maxautodownloadsize != m_maxAutoDownloadSize)
    {
        m_maxAutoDownloadSize = maxautodownloadsize;
        emit signalMaxAutoDownloadSizeChanged();
    }
}

/**
 * @brief EnvironmentMgr::availableEnvelopeTTL
 * @return
 */
QVariantList EnvironmentMgr::availableEnvelopeTTL() const
{
    return m_availableEnvelopeTTL;
}

/**
 * @brief EnvironmentMgr::setavailableEnvelopeTTL
 * @param availableenvelopettl
 */
void EnvironmentMgr::setAvailableEnvelopeTTL(const QVariantList &availableenvelopettl)
{
    if(availableenvelopettl != m_availableEnvelopeTTL)
    {
        QVariant max = -1;
        foreach(auto ttl, availableenvelopettl) {
            if(ttl > max && max != 0)
                max = ttl;
        }
        if(max < 0)
            max = 0;

        m_maxMessageTTL = max;
        //m_availableEnvelopeTTL = availableenvelopettl;
        emit onAvailableEnvelopeTTLChanged();
    }
}

/**
 * @brief EnvironmentMgr::availableEnvelopeBOR
 * @return
 */
QVariantList EnvironmentMgr::availableEnvelopeBOR() const
{
    return m_availableEnvelopeBOR;
}

/**
 * @brief EnvironmentMgr::setavailableEnvelopeBOR
 * @param destructonreadenvelope
 */
void EnvironmentMgr::setAvailableEnvelopeBOR(const QVariantList &destructonreadenvelope)
{
    if(destructonreadenvelope != m_availableEnvelopeBOR)
    {
        QVariant max = -1;
        foreach(auto bor, destructonreadenvelope) {
            if(bor > max && max != 0)
                max = bor;
        }
        if(max < 0)
            max = 0;

        m_maxMessageBOR = max;
        //m_availableEnvelopeBOR = destructonreadenvelope;
        emit onAvailableEnvelopeBORChanged();
    }
}

/**
 * @brief EnvironmentMgr::maxMessageSize
 * @return
 */
int EnvironmentMgr::maxMessageSize() const {
    return 10000;
}

/**
 * @brief EnvironmentMgr::maxConvoMembers
 * @return
 */
int EnvironmentMgr::maxConvoMembers() const {
    return 50;
}

/**
 * @brief EnvironmentMgr::requireEmailVerify
 * @return
 */
bool EnvironmentMgr::requireEmailVerify() const {
    return m_requireEmailVerify;
}

/**
 * @brief EnvironmentMgr::setRequireEmailVerify
 * @param value
 */
void EnvironmentMgr::setRequireEmailVerify(const bool& value) {
    m_requireEmailVerify = value;
}

/**
 * @brief EnvironmentMgr::requirePhoneVerify
 * @return
 */
bool EnvironmentMgr::requirePhoneVerify() const {
    return m_requirePhoneVerify;
}

/**
 * @brief EnvironmentMgr::setRequirePhoneVerify
 * @param value
 */
void EnvironmentMgr::setRequirePhoneVerify(const bool& value) {
    m_requirePhoneVerify = value;
}

/**
 * @brief EnvironmentMgr::requireVideoVerify
 * @return
 */
bool EnvironmentMgr::requireVideoVerify() const {
    return m_requireVideoVerify;
}

/**
 * @brief EnvironmentMgr::setRequireVideoVerify
 * @param value
 */
void EnvironmentMgr::setRequireVideoVerify(const bool& value) {
    m_requireVideoVerify = value;
}

EnvironmentMgr::VerificationMode EnvironmentMgr::verificationMode() const
{
#ifdef WICKR_BLACKOUT
    return VerificationNone;
#else
    return m_verificationMode;
#endif
}

void EnvironmentMgr::setVerificationMode(VerificationMode verMode)
{
    if (verMode != m_verificationMode) {
        m_verificationMode = verMode;
        emit verificationModeChanged();
    }
}

/**
 * @brief EnvironmentMgr::canChangePassword
 * @return
 */
bool EnvironmentMgr::canChangePassword() const
{
    return m_canChangePassword;
}

/**
 * @brief EnvironmentMgr::setCanChangePassword
 * @param canchangepassword
 */
void EnvironmentMgr::setCanChangePassword(const bool &canchangepassword)
{
    if(canchangepassword != m_canChangePassword)
    {
        m_canChangePassword = canchangepassword;
        emit onCanChangePasswordChanged();
    }
}

/**
 * @brief EnvironmentMgr::canResetPassword
 * @return
 */
bool EnvironmentMgr::canResetPassword() const
{
    return m_canResetPassword;
}

/**
 * @brief EnvironmentMgr::setCanResetPassword
 * @param canresetpassword
 */
void EnvironmentMgr::setCanResetPassword(const bool &canresetpassword)
{
    if(canresetpassword != m_canResetPassword)
    {
        m_canResetPassword = canresetpassword;
        emit signalCanResetPasswordChanged();
    }
}

/**
 * @brief EnvironmentMgr::canCreateRoom
 * @return
 */
bool EnvironmentMgr::canCreateRoom() const
{
    return m_canCreateRoom;
}

/**
 * @brief EnvironmentMgr::setCanCreateRoom
 * @param cancreateroom
 */
void EnvironmentMgr::setCanCreateRoom(const bool &cancreateroom)
{
    if(cancreateroom != m_canCreateRoom)
    {
        m_canCreateRoom = cancreateroom;
        emit signalCanCreateRoomChanged();
    }
}

/**
 * @brief EnvironmentMgr::canAddContact
 * @return
 */
bool EnvironmentMgr::canAddContact() const
{
    return m_canAddContact;
}

/**
 * @brief EnvironmentMgr::setCanAddContact
 * @param canaddcontact
 */
void EnvironmentMgr::setCanAddContact(const bool &canaddcontact)
{
    if(canaddcontact != m_canAddContact)
    {
        m_canAddContact = canaddcontact;
        emit onCanAddContactChanged();
    }
}

/**
 * @brief EnvironmentMgr::alwaysReauthenticate
 * @return
 */
bool EnvironmentMgr::alwaysReauthenticate() const
{
    return m_alwaysReauthenticate;
}

/**
 * @brief EnvironmentMgr::setAlwaysReauthenticate
 * @param alwaysreauthenticate
 */
void EnvironmentMgr::setAlwaysReauthenticate(const bool &alwaysreauthenticate)
{
    if(alwaysreauthenticate != m_alwaysReauthenticate)
    {
        m_alwaysReauthenticate = alwaysreauthenticate;
        emit signalAlwaysReauthenticateChanged();
    }
}

/**
 * @brief EnvironmentMgr::requireTransitiveTrust
 * @return
 */
bool EnvironmentMgr::requireTransitiveTrust() const
{
    return m_requireTransitiveTrust;
}

/**
 * @brief EnvironmentMgr::setRequireTransitiveTrust
 * @param requiretransitivetrust
 */
void EnvironmentMgr::setRequireTransitiveTrust(const bool &requiretransitivetrust)
{
    if(requiretransitivetrust != m_requireTransitiveTrust)
    {
        m_requireTransitiveTrust = requiretransitivetrust;
        emit onRequireTransitiveTrustChanged();
    }
}

/**
 * @brief EnvironmentMgr::userNameRules
 * @return
 */
QString EnvironmentMgr::userNameRules() const
{
    return m_userNameRules;
}

/**
 * @brief EnvironmentMgr::setUserNameRules
 * @param usernamerules
 */
void EnvironmentMgr::setUserNameRules(const QString &usernamerules)
{
    if(usernamerules != m_userNameRules)
    {
        m_userNameRules = usernamerules;
        emit signalUserNameRulesChanged();
    }
}

/**
 * @brief EnvironmentMgr::onlyShowInNetwork
 * @return
 */
bool EnvironmentMgr::onlyShowInNetwork() const
{
    return m_onlyShowInNetwork;
}

/**
 * @brief EnvironmentMgr::setOnlyShowInNetwork
 * @param onlyshowinnetwork
 */
void EnvironmentMgr::setOnlyShowInNetwork(const bool &onlyshowinnetwork)
{
    if(onlyshowinnetwork != m_onlyShowInNetwork)
    {
        m_onlyShowInNetwork = onlyshowinnetwork;
        emit signalOnlyShowInNetworkChanged();
    }
}

/**
 * @brief EnvironmentMgr::emailAsUserIdMode
 * @return
 */
bool EnvironmentMgr::emailAsUserIdMode() const
{
    return m_emailAsUserIdMode;
}

/**
 * @brief EnvironmentMgr::setEmailAsUserIdMode
 * @param emailasuseridmode
 */
void EnvironmentMgr::setEmailAsUserIdMode(const bool &emailasuseridmode)
{
    if(emailasuseridmode != m_emailAsUserIdMode)
    {
        m_emailAsUserIdMode = emailasuseridmode;
        emit signalEmailAsUserIdModeChanged();
    }

    if(!m_emailAsUserIdMode)
    {
        setOnlyShowInNetwork(true);
        setFriendFinder(false);
        setCanAddContact(false);
    }
}

/**
 * @brief EnvironmentMgr::networkToken
 * @return
 */
QString EnvironmentMgr::networkToken() const
{
    return m_networkToken;
}

/**
 * @brief EnvironmentMgr::setNetworkToken
 * @param networktoken
 */
void EnvironmentMgr::setNetworkToken(const QString &networktoken)
{
    if(networktoken != m_networkToken)
    {
        m_networkToken = networktoken;
        emit signalNetworkTokenChanged();
    }
}

/**
 * @brief EnvironmentMgr::loadJson
 * Loads user settings from JSON string buffer.
 * @param jsonStr
 * @return
 */
bool EnvironmentMgr::loadJson(const QString & jsonStr, bool saveSettings)
{
    if (jsonStr.isEmpty())
        return false;

    return loadJson(QJsonDocument::fromJson(jsonStr.toUtf8()).object(),saveSettings);
}

/**
 * @brief EnvironmentMgr::loadJson
 * Loads user settings from JSON object.
 * @param json
 * @return
 */
bool EnvironmentMgr::loadJson(const QJsonObject &json, bool saveSettings)
{
    if (json.isEmpty())
        return false;

    // Check update timestamp
    ulong updateTm = json[ENV_PERMMOD].toInt();
    if (m_lastUpdateTime && (m_lastUpdateTime > updateTm)) {
        qDebug().noquote().nospace() << "ENVIRONMENT MANAGER: Ignoring User Settings Update, same/older than last update."
                                     << "(Last: " << m_lastUpdateTime << " >= Update: " << updateTm << ")";
        return false;
    }

    qDebug().noquote().nospace() << "ENVIRONMENT MANAGER: Applying User Settings Update, DATA = " << json;
#if !defined(WICKR_SCIF) && !defined(WICKR_PRODUCTION)
    if (json.contains(ENV_STARTCALL))         { setCanStartCall(json[ENV_STARTCALL].toBool());                    }
#endif
    if (json.contains(ENV_FRIENDFINDER))      { setFriendFinder(json[ENV_FRIENDFINDER].toBool());                 }
#ifndef WICKR_SCIF
    if (json.contains(ENV_DEVICELOCKOUT))     { setForceDeviceLockout(json[ENV_DEVICELOCKOUT].toVariant());       }
#endif
    if (json.contains(ENV_MAXDOWNLOAD))       { setMaxAutoDownloadSize(json[ENV_MAXDOWNLOAD].toVariant());        }
    if (json.contains(ENV_MAXUPLOAD))         { setMaxUploadSize(json[ENV_MAXUPLOAD].toVariant()); }
    //if (json.contains(ENV_AVAILTTL))          { setAvailableEnvelopeTTL(json[ENV_AVAILTTL].toVariant().toList()); }
    //if (json.contains(ENV_AVAILBOR))          { setAvailableEnvelopeBOR(json[ENV_AVAILBOR].toVariant().toList()); }

#ifndef WICKR_SCIF
    if (json.contains(ENV_MAXTTL))            { setMaxMessageTTL(json[ENV_MAXTTL].toVariant()); }
    if (json.contains(ENV_MAXBOR))            { setMaxMessageBOR(json[ENV_MAXBOR].toVariant()); }
    if (json.contains(ENV_CHANGEPASSWORD))    { setCanChangePassword(json[ENV_CHANGEPASSWORD].toBool());          }
#endif
    if (json.contains(ENV_RESETPASSWORD))     { setCanResetPassword(json[ENV_RESETPASSWORD].toBool());            }
    if (json.contains(ENV_TRANSITIVETRUST))   { setRequireTransitiveTrust(json[ENV_TRANSITIVETRUST].toBool());    }
#ifndef WICKR_SCIF
    if (json.contains(ENV_REAUTHENTICATE))    { setAlwaysReauthenticate(json[ENV_REAUTHENTICATE].toBool());       }
#endif
    if (json.contains(ENV_NAMERULES))         { setUserNameRules(json[ENV_NAMERULES].toString()); }
#ifndef WICKR_SCIF
    if (json.contains(ENV_ADDCONTACT))        { setCanAddContact(json[ENV_ADDCONTACT].toBool()); }
#endif
    if (json.contains(ENV_ONLYSHOWINNETWORK)) { setOnlyShowInNetwork(json[ENV_ONLYSHOWINNETWORK].toBool());       }
#ifndef WICKR_SCIF
    if (json.contains(ENV_CREATEROOM))        { setCanCreateRoom(json[ENV_CREATEROOM].toBool()); }
#endif
    if (json.contains(ENV_VERIFICATIONMODE))  { setVerificationMode(static_cast<EnvironmentMgr::VerificationMode>(json[ENV_VERIFICATIONMODE].toInt())); }

    m_lastUpdateTime = updateTm;

    // Update environment json
    updateEnvironment();

    // Save environment (if option set)
    WickrCore::WickrSession *session = WickrCore::WickrSession::getActiveSession();
    if (saveSettings && session) {
        session->account->setEnvironment(QJsonDocument(m_permissions).toJson());
        session->account->save();
    }
    return true;
}

/**
 * @brief EnvironmentMgr::loadBootStrapJson
 * @param jsonDoc
 * @return
 */
bool EnvironmentMgr::loadBootStrapJson(const QJsonDocument & jsonDoc)
{
    if(jsonDoc.isEmpty()) {
        return false;
    }

    if (!m_hasConfiguration) {
        m_hasConfiguration = true;
        emit signalHasConfigurationChanged();
    }

    // These are defaults for our on-prem customers
    setEmailAsUserIdMode(false);
    setCanStartCall(false);
    setRequireEmailVerify(false);
    setRequirePhoneVerify(false);
    setRequireVideoVerify(false);
    setRequireTransitiveTrust(false);
    setForceDeviceLockout(5);
    m_defaultDeviceLockout = 5;

    QJsonObject jsonObject = jsonDoc.object();

    if(jsonObject.contains("usernameMode")) {
        switch(jsonObject.value("usernameMode").toVariant().toInt()) {
        case 0:
            setEmailAsUserIdMode(false);
            break;
        case 1:
            setEmailAsUserIdMode(true);
            break;
        }
    }

    if(jsonObject.contains("requireEmailVerification"))
        setRequirePhoneVerify(jsonObject.value("requireEmailVerification").toVariant().toBool());

    if(jsonObject.contains("requirePhoneVerification"))
        setRequirePhoneVerify(jsonObject.value("requirePhoneVerification").toVariant().toBool());

    if(jsonObject.contains("requireVideoVerification"))
        setRequirePhoneVerify(jsonObject.value("requireVideoVerification").toVariant().toBool());

    if(jsonObject.contains("userManagedCredentials")) {
        if(jsonObject.value("userManagedCredentials").toVariant().toBool()) {
            setCanChangePassword(true);
            setCanResetPassword(true);
        }
        else {
            setCanChangePassword(false);
            setCanResetPassword(false);
        }
    }

    if(jsonObject.contains("serviceSSLKeys")) {
        qDebug() << jsonObject.value("serviceSSLKeys").toVariant().toStringList();

        QStringList keys = jsonObject.value("serviceSSLKeys").toVariant().toStringList();
        m_certs.append(keys);

        QList<QSslCertificate> clist;
        foreach(QString key, keys) {
            if(key.length() < 1)
                continue;

            // Create a certificate object
            if(!key.contains("-----BEGIN CERTIFICATE-----"))
                key = "-----BEGIN CERTIFICATE-----\n" + key + "\n-----END CERTIFICATE-----";
            else
                key = key.replace(QString("\\n"), QString(QChar('\n')));
            clist << QSslCertificate(key.toUtf8());
        }
        pin(clist);
    }

    if(jsonObject.contains("serviceRegKeys")) {
        qDebug() << jsonObject.value("serviceRegKeys").toVariant().toStringList();

        QStringList keys = jsonObject.value("serviceRegKeys").toVariant().toStringList();
        QStringList updatedKeys;

        foreach(QString key, keys) {
            if(key.length() < 1)
                continue;

            // Create a certificate object
            if(!key.contains("-----BEGIN PUBLIC KEY-----"))
                key = "-----BEGIN PUBLIC KEY-----\n" + key + "\n-----END PUBLIC KEY-----";

            updatedKeys.append(key);
        }

        m_keys.append(updatedKeys);

        if (!m_keys.isEmpty()) {
            if (!cryptoServerRegKeys(m_keys)) {
                qDebug() << "Failed to set Server Registration Keys!";
            }
        }
    }

    m_certs.removeDuplicates();
    m_keys.removeDuplicates();

    if(jsonObject.contains("regToken")) {
       m_networkToken = jsonObject.value("regToken").toVariant().toString();
    }

    if(jsonObject.contains("serviceHost")) {
        QString host = jsonObject.value("serviceHost").toVariant().toString();
        if(!host.isEmpty()) {
            QString apipath = "/116/src";
            QString baseURL = "https://" + host + apipath;

            WickrURLs::setBaseURL(baseURL);
            qDebug() << "*** BASE URL" << baseURL;
        }
    }

    return true;
}

bool EnvironmentMgr::isPinned(QList<QSslCertificate> peers)
{
    if(m_pinned.count() < 1 || peers.count() < 1)
        return true;

    foreach(auto peer, peers) {
        foreach(auto cert, m_pinned) {
            if(peer == cert)
                return true;
        }
    }
    return false;
}

void EnvironmentMgr::pin(QList<QSslCertificate> list)
{
    qDebug() << "*** API Pinning";
    m_pinned = list;
    if(list.count() > 0) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setCaCertificates(list);
        QSslConfiguration::setDefaultConfiguration(config);
    }
}

/**
 * @brief EnvironmentMgr::updateEnvironment
 * Build/Rebuild Json from current environment settings.
 */
void EnvironmentMgr::updateEnvironment()
{
    m_permissions[ENV_PERMMOD] = (int)m_lastUpdateTime;
    m_permissions[ENV_STARTCALL] = m_canStartCall;
    m_permissions[ENV_FRIENDFINDER] = m_friendFinder;
    m_permissions[ENV_DEVICELOCKOUT] = m_forceDeviceLockout.toInt();
    m_permissions[ENV_MAXDOWNLOAD] = m_maxAutoDownloadSize.toInt();
    m_permissions[ENV_MAXUPLOAD] = m_maxUploadSize.toInt();
    m_permissions[ENV_AVAILTTL] = QJsonArray::fromVariantList(m_availableEnvelopeTTL);
    m_permissions[ENV_AVAILBOR] = QJsonArray::fromVariantList(m_availableEnvelopeBOR);
    m_permissions[ENV_CHANGEPASSWORD] = m_canChangePassword;
    m_permissions[ENV_RESETPASSWORD] = m_canResetPassword;
    m_permissions[ENV_REAUTHENTICATE] = m_alwaysReauthenticate;
    m_permissions[ENV_NAMERULES] = m_userNameRules;
    m_permissions[ENV_ADDCONTACT] = m_canAddContact;
    m_permissions[ENV_ONLYSHOWINNETWORK] = m_onlyShowInNetwork;
    m_permissions[ENV_CREATEROOM] = m_canCreateRoom;
    m_permissions[ENV_VERIFICATIONMODE] = m_verificationMode;
}

/**
 * @brief EnvironmentMgr::clearEnvironment
 */
void EnvironmentMgr::clearEnvironment()
{
    if (m_hasConfiguration) {
        m_hasConfiguration = false;
        emit signalHasConfigurationChanged();
    }

    // clear last update time
    m_lastUpdateTime = 0;

    m_certs.clear();
    m_keys.clear();
    m_networkToken.clear();
    m_pinned.clear();

    WickrURLs::setBaseURL("");

    // Reset the SSL certificates
    QSslConfiguration::setDefaultConfiguration(QSslConfiguration::defaultConfiguration());

    // Reset the server keys
    if (!cryptoServerRegKeys(QStringList())) {
        qDebug() << "Failed to reset Server Registration Keys!";
    }

    m_friendFinder = true;
    m_maxUploadSize.clear();
    m_maxAutoDownloadSize.clear();

#if defined(WICKR_BLACKOUT)
    QVariantList defaultTTLs;
    defaultTTLs.append(300);      // 5 minutes
    defaultTTLs.append(3600);     // 1 hour
    defaultTTLs.append(43200);    // 12 hours
    defaultTTLs.append(86400);    // 1 day
    defaultTTLs.append(172800);   // 2 days
    defaultTTLs.append(86400 * 6);   // 6 day

    QVariantList defaultBORs;
    defaultBORs.append(60);       // 1 minutes
    defaultBORs.append(300);      // 5 minutes
    defaultBORs.append(600);      // 10 minutes
    defaultBORs.append(1800);     // 30 minutes
    defaultBORs.append(3600);     // 1 hour
    defaultBORs.append(86400);    // 1 day

    m_availableEnvelopeBOR = defaultBORs;
    m_availableEnvelopeTTL = defaultTTLs;
    m_maxMessageBOR = 86400;
    m_maxMessageTTL = 86400 * 6;
    m_defaultMessageTTL = 172800;
    m_defaultMessageBOR = 300;
#elif defined(WICKR_SCIF)
    QVariantList defaultTTLs;
    defaultTTLs.append(3600 * 6);       // 6 hours
    defaultTTLs.append(3600 * 12);      // 12 hours
    defaultTTLs.append(86400);          // 1 day
    defaultTTLs.append(86400 * 2);      // 2 days
    defaultTTLs.append(86400 * 3);      // 3 days
    defaultTTLs.append(86400 * 6);      // 6 days

    QVariantList defaultBORs;
    defaultBORs.append(0);              // is ignored
    defaultBORs.append(10);             // 10 seconds
    defaultBORs.append(60);             // 1 minute
    defaultBORs.append(600);            // 10 minutes
    defaultBORs.append(1800);           // 30 minutes
    defaultBORs.append(3600);           // 1 hour
    defaultBORs.append(3600 * 6);       // 6 hours

    m_availableEnvelopeBOR = defaultBORs;
    m_availableEnvelopeTTL = defaultTTLs;
    m_maxMessageBOR = m_defaultMessageBOR = 0;
    m_maxMessageTTL = m_defaultMessageTTL = 86400 * 6;
#else
    QVariantList defaultTTLs;
    defaultTTLs.append(86400 * 7);   // 1 week
    defaultTTLs.append(86400 * 14);  // 2 week
    defaultTTLs.append(86400 * 30);  // 30 days
    defaultTTLs.append(86400 * 60);  // 60 days
    defaultTTLs.append(86400 * 90);  // 90 days
    defaultTTLs.append(86400 * 180); // 180 days
    defaultTTLs.append(86400 * 365); // 1 year

    QVariantList defaultBORs;
    defaultBORs.append(0);              // is ignored
    defaultBORs.append(10);             // 10 seconds
    defaultBORs.append(60);             // 1 minute
    defaultBORs.append(600);            // 10 minutes
    defaultBORs.append(1800);           // 30 minutes
    defaultBORs.append(3600);           // 1 hour
    defaultBORs.append(3600 * 6);       // 6 hours

    m_availableEnvelopeBOR = defaultBORs;
    m_availableEnvelopeTTL = defaultTTLs;
    m_maxMessageBOR = m_defaultMessageBOR = 0;
    m_defaultMessageTTL = 86400 * 7;
    m_maxMessageTTL = 86400 * 365;
#endif

    setCanChangePassword(true);
    setCanResetPassword(true);
    setCanCreateRoom(true);
    setCanAddContact(true);
    setAlwaysReauthenticate(false);
    setUserNameRules("^[a-zA-Z0-9]*$");

    setEmailAsUserIdMode(true);
    setCanStartCall(true);
    setRequireEmailVerify(true);
    setRequirePhoneVerify(true);
    setRequireVideoVerify(true);
    setRequireTransitiveTrust(true);
}

/**
 * @brief EnvironmentMgr::getRemainingStringFromTTL
 * @param ttl
 * @return
 */
QString EnvironmentMgr::getRemainingStringFromTTL(const int ttl)
{
    QString rval;
    if(ttl == -1) {
        rval = "Disabled";
    }
    else if(ttl == 0) {
        rval = "Off";
    }
    else if(ttl >= 86400) {
        int days = ttl/86400;

        rval = QString::number(days) + QString(" day");
        if(days > 1) {
            rval += "s";
        }
    }
    else if(ttl >= 3600) {
        int hour = ttl/3600;
        rval = QString::number(hour) + QString(" hour");
        if(hour > 1)
        {
            rval += "s";
        }
    }
    else if ( ttl >= 60 ) {
        int minutes = ttl/60;

        rval = QString::number(minutes) + QString(" minute");
        if(minutes > 1)
        {
            rval += "s";
        }
    }
    else {
        rval = QString::number(ttl) + QString(" seconds");
    }

    return rval;
}

/**
 * @brief EnvironmentMgr::isTTLValid
 * @param ttl
 * @return
 */
bool EnvironmentMgr::isTTLValid(long ttl)
{
    if(m_maxMessageTTL < 1)
        return true;

    if((ttl > 0) && (QVariant(static_cast<qlonglong>(ttl)) <= m_maxMessageTTL))
        return true;

    return false;
}

/**
 * @brief EnvironmentMgr::calculateNewTTl
 * @param ttl
 * @return
 */
long EnvironmentMgr::calculateNewTTl(long ttl)
{
    if(m_maxMessageTTL < 1)
        return ttl;
    if(ttl > 0 && QVariant(static_cast<qlonglong>(ttl)) <= m_maxMessageTTL)
        return ttl;
    return static_cast<long>(m_maxMessageTTL.toLongLong());
}

/**
 * @brief EnvironmentMgr::isBORValid
 * @param bor
 * @return
 */
bool EnvironmentMgr::isBORValid(long bor)
{
    if(m_maxMessageBOR < 1)
        return true;

    if((bor > 0) && (QVariant(static_cast<qlonglong>(bor)) <= m_maxMessageBOR))
        return true;

    return false;
}

/**
 * @brief EnvironmentMgr::calculateNewBOR
 * @param bor
 * @return
 */
long EnvironmentMgr::calculateNewBOR(long bor)
{
    if(m_maxMessageBOR < 1)
        return bor;
    if(bor > 0 && QVariant(static_cast<qlonglong>(bor)) <= m_maxMessageBOR)
        return bor;
    return static_cast<long>(m_maxMessageBOR.toLongLong());
}

void EnvironmentMgr::storeNetworkConfig(QSettings &settings)
{
    settings.setValue("baseURL", WickrURLs::getBaseURL());
    settings.setValue("certs", getCerts());
    settings.setValue("keys", getKeys());
    settings.setValue("networkToken", networkToken());
    settings.sync();
}

bool EnvironmentMgr::loadNetworkConfig(const QSettings &settings)
{
    if (!settings.contains("baseURL") && !settings.contains("DefaultBaseURL") &&
            !settings.contains("certs") && !settings.contains("keys") && !settings.contains("networkToken"))
    {
        return false;
    }

    QString baseurl = settings.value("baseURL").toString();
    if (baseurl.isEmpty()) {
        baseurl = settings.value("DefaultBaseURL").toString();  // Legacy name that existing customers might be using
    }
    if(!baseurl.isEmpty())
    {
        WickrURLs::setBaseURL(baseurl);
    }

    QStringList certs = settings.value("certs").toStringList();
    QSslCertificate skey;
    QList<QSslCertificate> clist;
    if(!certs.isEmpty()) {
        foreach(QString key, certs) {
            if(key.length() < 1)
                continue;
            // Create a certificate object
            if(!key.contains("-----BEGIN CERTIFICATE-----"))
                key = "-----BEGIN CERTIFICATE-----\n" + key + "\n-----END CERTIFICATE-----";
            else
                key = key.replace(QString("\\n"), QString(QChar('\n')));
            skey = QSslCertificate(key.toUtf8());
            if(skey.isBlacklisted() || skey.isNull())
                qDebug() << "*** Invalid certificate used";
            clist.append(skey);
        }
        pin(clist);
    }

    QStringList regKeys = settings.value("keys").toStringList();
    if (!regKeys.isEmpty()) {
        if (!cryptoServerRegKeys(regKeys)) {
            qDebug() << "Failed to set Server Registration Keys!";
        }
    }

    QString networkToken = settings.value("networkToken").toString();
    if (!networkToken.isEmpty()) {
        setNetworkToken(networkToken);
    }

    return true;
}

