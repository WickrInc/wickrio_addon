#ifndef CMDOPERATION_H
#define CMDOPERATION_H

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QSettings>
#include "wickriodatabase.h"
#include "wickrIOAppSettings.h"
#include "operationdata.h"

class CmdOperation : public QObject
{
    Q_OBJECT
public:
    explicit CmdOperation(OperationData *operation, QObject *parent=0);

    bool openDatabase();

public:
    QSettings *m_settings;
    QString m_dbLocation;
    QString m_appNm;
    WickrIOClientDatabase *m_ioDB;

    // Some applications rely on this class to maintain DB stuff
    OperationData* m_operation;
};

#endif // CMDOPERATION_H
