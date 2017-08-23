#ifndef CMDPARSER_H
#define CMDPARSER_H

#include <QObject>

#include <QString>
#include <QSettings>

#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrioappsettings.h"
#include "wickrioparsers.h"
#include "cmdbase.h"
#include "cmdoperation.h"
#include "cmdconsole.h"

#include <QProcess>

class CmdParser : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdParser(CmdOperation *operation);

    bool runCommands();
    void status();

private:
    CmdOperation *m_operation;

    // Parser Message handling values
    bool m_parserMsgSuccess;
    bool m_parserMsgInProcess;

    QProcess *m_exec;

    QList<WickrIOParsers *> m_parsers;

    bool getParserValues(WickrIOParsers *parser);

    void addParser();
    void deleteParser(int parserIndex);
    void listParsers();
    void modifyParser(int parserIndex);
    void pauseParser(int parserIndex);
    void startParser(int parserIndex);

    bool chkParsersNameExists(const QString& name);

    bool validateIndex(int parserIndex);

    bool sendParserCmd(int port, const QString& cmd);

private slots:
    void slotCmdFinished(int, QProcess::ExitStatus);
    void slotCmdOutputRx();

};

#endif // CMDPARSER_H
