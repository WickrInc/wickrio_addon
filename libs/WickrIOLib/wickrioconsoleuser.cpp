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

void WickrIOConsoleUser::setEdit(bool edit)
{
    if (edit) {
        permissions |= CUSER_PERM_EDIT_FLAG;
    } else {
        permissions &= ~CUSER_PERM_EDIT_FLAG;
    }
}

void WickrIOConsoleUser::setCreate(bool create)
{
    if (create) {
        permissions |= CUSER_PERM_CREATE_FLAG;
    } else {
        permissions &= ~CUSER_PERM_CREATE_FLAG;
    }
}

void WickrIOConsoleUser::setRxEvents(bool rxEvents)
{
    if (rxEvents) {
        permissions |= CUSER_PERM_EVENTS_FLAG;
    } else {
        permissions &= ~CUSER_PERM_EVENTS_FLAG;
    }
}
