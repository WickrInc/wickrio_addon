#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QJsonArray>
#include <QString>
#include "createjson.h"

CreateJsonAction::CreateJsonAction() :
    m_has_bor(false)
{
    action = "";
    name = "";
    users.clear();
    ttl = 0;
    message = "";
    attachments.clear();
}

CreateJsonAction::CreateJsonAction(QString action, QStringList users, int ttl, QString message, QList<QString> attachments) :
    m_has_bor(false)
{
    this->action = action;
    this->users = users;
    this->name.clear();
    this->ttl = ttl;
    this->message = message;
    if (attachments.isEmpty()) {
        this->attachments.clear();
    } else {
        this->attachments = attachments;
    }
}


CreateJsonAction::CreateJsonAction(QString action, QString name, int ttl, QString message, QList<QString> attachments, bool isVGroupID) :
    m_has_bor(false)
{
    this->action = action;
    if (isVGroupID) {
        this->vgroupid = name;
    } else {
        this->name = name;
    }
    this->users.clear();
    this->ttl = ttl;
    this->message = message;
    if (attachments.isEmpty()) {
        this->attachments.clear();
    } else {
        this->attachments = attachments;
    }
}

CreateJsonAction::~CreateJsonAction()
{
}

void
CreateJsonAction::setBOR(int bor)
{
    m_bor = bor;
    m_has_bor = true;
}

QByteArray
CreateJsonAction::toByteArray()
{
    QList<QString> ids;

    QJsonObject jsonObject;

    jsonObject.insert("action", this->action);

    if (users.size() > 0) {
        QJsonArray usersArray;
        foreach (QString user , this->users) {
            QJsonObject userObject;
            userObject.insert("name", user);
            usersArray.append(userObject);
        }
        jsonObject.insert("users", usersArray);
    } else if (! name.isEmpty()) {
        jsonObject.insert("user", this->name);
    } else if (! vgroupid.isEmpty()) {
        jsonObject.insert("vgroupid", this->vgroupid);
    }

    if (this->attachments.size() > 0) {
        QJsonArray arrayValue;
        for (int i=0; i<attachments.size(); i++) {
            QString attachment = attachments.at(i);
            arrayValue.append(attachment);
        }
        jsonObject.insert("attachments", arrayValue);
    }
    if (this->message != NULL) {
        jsonObject.insert("message", this->message);
    }

    if (this->ttl > 0) {
        jsonObject.insert("ttl", this->ttl);
    }

    if (m_has_bor) {
        jsonObject.insert("bor", m_bor);
    }

    QJsonObject operationObject;
    operationObject.insert("operation", jsonObject);

    QJsonDocument saveDoc(operationObject);

    QByteArray byteArray = saveDoc.toJson();

    return byteArray;
}
