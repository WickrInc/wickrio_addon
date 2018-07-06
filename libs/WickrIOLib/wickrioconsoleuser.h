#ifndef WICKRIOCONSOLEUSER_H
#define WICKRIOCONSOLEUSER_H

#include <QString>

#include "wickrbotlib.h"

#define CUSER_PERM_ADMIN_FLAG   0x01
#define CUSER_PERM_EDIT_FLAG    0x02
#define CUSER_PERM_CREATE_FLAG  0x04
#define CUSER_PERM_EVENTS_FLAG  0x08

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
    bool canEdit() { return permissions & CUSER_PERM_EDIT_FLAG; }
    void setEdit(bool edit);
    bool canCreate() { return permissions & CUSER_PERM_CREATE_FLAG; }
    void setCreate(bool create);
    bool rxEvents() { return permissions & CUSER_PERM_EVENTS_FLAG; }
    void setRxEvents(bool rxEvents);

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
