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
#if 0
    QTextStream input(stdin);

    while (true) {
        qDebug() << "CONSOLE:Enter one of [client, advanced, server, console, parser or users]:";
        QString line = input.readLine();
        qDebug() << "CONSOLE: Received: " << line;
    }
#else
    CmdMain cmdmain(m_operation);
    cmdmain.runCommands();
#endif
}

void
WickrIOProcessCommand::processFinished()
{

}
