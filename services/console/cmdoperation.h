#ifndef CMDOPERATION_H
#define CMDOPERATION_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include "wickrbotipc.h"
#include "wickriodatabase.h"
#include "wickrioappsettings.h"

class CmdOperation : public QObject
{
    Q_OBJECT
public:
    explicit CmdOperation(QObject *parent=0);

    bool openDatabase();

public:
    QSettings *m_settings;
    QString m_dbLocation;
    QString m_appNm;
    WickrBotIPC *m_ipc;
    WickrIOClientDatabase *m_ioDB;

};

#endif // CMDOPERATION_H
