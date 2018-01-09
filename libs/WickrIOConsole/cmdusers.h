#ifndef CMDUSERS_H
#define CMDUSERS_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "cmdbase.h"
#include "cmdoperation.h"

class CmdUsers : public CmdBase
{
    Q_OBJECT

public:
    explicit CmdUsers(CmdOperation *operation);
    ~CmdUsers();

    bool runCommands();
    void status();

private:
    void addUser();
    void deleteUser(int index);
    void listUsers();
    void modifyUser(int index);
    bool validateIndex(int index);

    bool getUserValues(WickrIODBUser *cuser);
    bool getUserClients(WickrIODBUser *user);

private:
    CmdOperation *m_operation;
    QList<WickrIODBUser *> m_users;
};

#endif // CMDUSERS_H
