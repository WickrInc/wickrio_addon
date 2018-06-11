#ifndef WICKRIOPROCESSCOMMAND_H
#define WICKRIOPROCESSCOMMAND_H

#include <QObject>
#include <QThread>
#include <QString>

#include <operationdata.h>

#define WICKRIOPROCESSCOMMAND WickrIOProcessCommand::theProcessCommand

class WickrIOProcessCommand : public QThread
{
Q_OBJECT

public:
    WickrIOProcessCommand(OperationData *pOperation=nullptr);

    static WickrIOProcessCommand *theProcessCommand;

private:
    OperationData *m_operation=nullptr;

private slots:
    void processStarted();
    void processFinished();

};

#endif // WICKRIOPROCESSCOMMAND_H
