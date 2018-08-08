#ifndef CMDPARSER_H
#define CMDPARSER_H

#include <QObject>
#include <QString>
#include <QSettings>

#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "wickrIOParsers.h"
#include "cmdbase.h"
#include "cmdoperation.h"
#include "cmdconsole.h"

#include <QProcess>


class CmdParser : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdParser(CmdOperation *operation);

    bool runCommands(QString commands);
    void status();

private:
    CmdOperation *m_operation;

    // Parser Message handling values
    bool m_parserMsgSuccess;
    bool m_parserMsgInProcess;


    QList<WickrIOParsers *> m_parsers;

    bool getParserValues(WickrIOParsers *parser);

    void addParser();
    void deleteParser(int parserIndex);
    void listParsers();
    void modifyParser(int parserIndex);
    void pauseParser(int parserIndex);
    void startParser(int parserIndex);

    bool chkParsersNameExists(const QString& name);
    bool chkParsersInterfaceExists(const QString &name, int port);

    bool validateIndex(int parserIndex);

    bool sendParserCmd(const QString& dest, const QString& cmd);

};

#endif // CMDPARSER_H
