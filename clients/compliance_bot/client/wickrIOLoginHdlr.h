#ifndef WICKRIOLOGINHDLR_H
#define WICKRIOLOGINHDLR_H

#include <QObject>
#include <QString>

#include "common/wickrRequests.h"
#include "session/wickrPreRegistrationIface.h"

#include "operationdata.h"

class WickrBotLogin {
public:
    QString m_name;
    QString m_pass;
    QString m_userName;
    int m_sent;
    int m_failedLogin;
    bool m_creating;

    WickrBotLogin(QString name, QString pass, QString userName) :
        m_name(name),
        m_pass(pass),
        m_userName(userName),
        m_creating(false) {}
};

typedef enum { LoggedOut, InProcess, LoggedIn, LoggingOut, LoginsFailed } WickrIOLoginState;


class WickrIOLoginHdlr : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOLoginHdlr(OperationData *operation);
    ~WickrIOLoginHdlr();

    void initiateLogin();

    WickrIOLoginState getLoginState() { return m_loginState; }

    /**
     * @brief addLogin
     * Add a user to the list of users used to login with.
     * @param user The username of the new userSS
     * @param pass The password for the new user
     */
    void addLogin(const QString& user, const QString& pass, const QString& userName) {
        m_logins.append(new WickrBotLogin(user, pass, userName));
    }


    void preRegistrationInit();
    QStringList preRegistrationGetKeyStrings();

private:
    OperationData *m_operation;

    int m_curLoginIndex;
    WickrIOLoginState m_loginState;
    QList<WickrBotLogin *> m_logins;
    int m_consecutiveLoginFailures;
    bool m_firstLogin;
    long m_backupVersion;

    void loginNextUser();
    void refreshDirectory();



    WickrCore::WickrPreRegistrationIface *m_preRegDataIface;


    void registerUser(const QString &wickrid, const QString &password, const QString &transactionid, bool newUser, bool sync, bool isRekey);

signals:
    void signalExit();
    void signalLoginFailed();
    void signalLoginSuccess(QString userSigningKey);
    void signalOnlineFlag(bool);

    void signalNetworkStatus(bool);

private slots:
    void slotRegistrationDone(WickrRegisterUserContext *c);
    void slotLoginDone(WickrLoginContext *ls);

    void slotRefreshDirectoryDone(WickrDirectoryGetContext* context);

    void slotLoginStart(const QString& username, const QString& password);

};

#endif // WICKRIOLOGINHDLR_H
