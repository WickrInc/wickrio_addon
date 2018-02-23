#ifndef CMDSERVER_H
#define CMDSERVER_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "cmdbase.h"
#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "cmdoperation.h"
#include "consoleserver.h"

class CmdServer : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdServer(CmdOperation *operation);
    ~CmdServer() {}

    bool runCommands(QString commands=QString());
    void status();

private:
    CmdOperation *  m_cmdOperation;
    ConsoleServer * m_consoleServer;
};

#endif // CMDSERVER_H
