#ifndef CMDADVANCED_H
#define CMDADVANCED_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "cmdoperation.h"
#include "cmdbase.h"

class CmdAdvanced : public CmdBase
{
    Q_OBJECT
public:
    explicit CmdAdvanced(CmdOperation *operation);

    bool runCommands(QString commands=QString());
    void status();

private:
    void configure();
    bool configEmail(WickrIOEmailSettings *email);
    bool configSSL(WickrIOSSLSettings *ssl);

private:
    CmdOperation *m_operation;
};

#endif // CMDADVANCED_H
