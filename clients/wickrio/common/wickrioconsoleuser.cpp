#include "wickrioconsoleuser.h"

WickrIOConsoleUser::WickrIOConsoleUser() :
    token("")
{

}

void WickrIOConsoleUser::setAdmin(bool admin)
{
    if (admin) {
        permissions |= CUSER_PERM_ADMIN_FLAG;
    } else {
        permissions &= ~CUSER_PERM_ADMIN_FLAG;
    }
}
