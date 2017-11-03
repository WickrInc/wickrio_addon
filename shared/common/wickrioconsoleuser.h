#ifndef WICKRIOCONSOLEUSER_H
#define WICKRIOCONSOLEUSER_H

#include <QString>

#include "wickrbotlib.h"

#define CUSER_PERM_ADMIN_FLAG   0x1

#define CUSER_AUTHTYPE_BASIC    0
#define CUSER_AUTHTYPE_EMAIL    1

class WickrIOConsoleUser //DECLSPEC
{
public:
    WickrIOConsoleUser();

    QString getAuthTypeStr() {
        if (authType == CUSER_AUTHTYPE_EMAIL) {
            return QString("Email");
        } else {
            return QString("Basic");
        }
    }

    bool isAdmin() { return(permissions & CUSER_PERM_ADMIN_FLAG); }
    void setAdmin(bool admin);

    int id;
    QString user;
    QString password;
    int permissions;
    int maxclients = 0;

    // Autthentication
    int authType;
    QString email;
    QString token;      //TODO: Temporary
};

#endif // WICKRIOCONSOLEUSER_H
