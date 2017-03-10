#ifndef CMDMAIN_H
#define CMDMAIN_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>

#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrioappsettings.h"
#include "cmdbase.h"
#include "cmdoperation.h"
#include "cmdclient.h"
#include "cmdadvanced.h"
#include "cmdconsole.h"
#include "cmdserver.h"

class CmdMain : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdMain();

    void runCommands();

private:
    CmdOperation m_operation;
    CmdClient    m_cmdClient;
    CmdConsole   m_cmdConsole;
    CmdAdvanced  m_cmdAdvanced;
    CmdServer    m_cmdServer;
};

#endif // CMDMAIN_H
