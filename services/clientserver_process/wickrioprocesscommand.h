#ifndef WICKRIOPROCESSCOMMAND_H
#define WICKRIOPROCESSCOMMAND_H

#include <QObject>
#include <QThread>
#include <QString>

#define WICKRIOPROCESSCOMMAND WickrIOProcessCommand::theProcessCommand

class WickrIOProcessCommand : public QThread
{
Q_OBJECT

public:
    WickrIOProcessCommand();

    static WickrIOProcessCommand *theProcessCommand;

private slots:
    void processStarted();
    void processFinished();

};

#endif // WICKRIOPROCESSCOMMAND_H
