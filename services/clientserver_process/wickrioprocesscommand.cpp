#include <QTextStream>
#include <QDebug>

#include "wickrioprocesscommand.h"
#if 1
#include "cmdclient.h"
#else
#include "cmdmain.h"
#endif

WickrIOProcessCommand *WickrIOProcessCommand::theProcessCommand;

WickrIOProcessCommand::WickrIOProcessCommand(OperationData *pOperation) :
    m_operation(pOperation)
{
    moveToThread(this);
    connect(this, &WickrIOProcessCommand::started, this, &WickrIOProcessCommand::processStarted, Qt::QueuedConnection);
    connect(this, &WickrIOProcessCommand::finished, this, &WickrIOProcessCommand::processFinished, Qt::QueuedConnection);
}

void
WickrIOProcessCommand::processStarted()
{
#if 1
    CmdOperation    cmdOperation(m_operation);
    CmdClient       cmdClient(&cmdOperation);
    QStringList     options;

    // Make the client commands the root
    options.append("-root");
    options.append("-basic");

    // Print out a list of the clients
    cmdClient.runCommands(options, "list");
    cmdClient.runCommands(options);
#else
    CmdMain cmdmain(m_operation);

    // Print out a list of the clients
    cmdmain.runCommands("client -basic,list");
    cmdmain.runCommands();
#endif

    emit signalQuit();
}

void
WickrIOProcessCommand::processFinished()
{
}
