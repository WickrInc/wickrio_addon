#include <QTextStream>
#include <QDebug>

#include "wickrioprocesscommand.h"
#include "cmdmain.h"

WickrIOProcessCommand *WickrIOProcessCommand::theProcessCommand;

WickrIOProcessCommand::WickrIOProcessCommand()
{
    connect(this, &WickrIOProcessCommand::started, this, &WickrIOProcessCommand::processStarted);
    connect(this, &WickrIOProcessCommand::finished, this, &WickrIOProcessCommand::processFinished);
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
    CmdMain cmdmain;
    cmdmain.runCommands();
#endif
}

void
WickrIOProcessCommand::processFinished()
{

}
