#ifndef CREATEJSON_H
#define CREATEJSON_H

#include <QString>
#include "wickrbotlib.h"

class DECLSPEC CreateJsonAction
{
public:
    CreateJsonAction();
    CreateJsonAction(QString action, QStringList users, long ttl, QString message, QList<QString> attachment);
    CreateJsonAction(QString action, QString name, long ttl, QString message, QList<QString> attachment, bool isVGroupID = false);
    ~CreateJsonAction();

    void setBOR(int bor);

public:
    QString action;
    QStringList users;
    QString name;
    QString vgroupid;
    long ttl;
    long m_bor;
    bool m_has_bor;
    QString message;
    QList<QString> attachments;

public:
    QByteArray toByteArray();
};

#endif // CREATEJSON_H
