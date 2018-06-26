#include <QTextStream>
#include <QDebug>

#include "wickrioprocesscommand.h"
#include "cmdmain.h"

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
    CmdMain cmdmain(m_operation);

    // Print out a list of the clients
    cmdmain.runCommands("client,list");
    cmdmain.runCommands();

    emit signalQuit();
}

void
WickrIOProcessCommand::processFinished()
{
}
