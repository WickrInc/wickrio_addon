#ifndef WICKRBOTJSON_H
#define WICKRBOTJSON_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

#define JSON_OBJECT_OPERATION       "operation"
#define JSON_OBJECT_ACTION          "action"
#define JSON_OBJECT_USER_LIST       "users"
#define JSON_OBJECT_USER_ID         "id"
#define JSON_OBJECT_USER_NAME       "name"
#define JSON_OBJECT_USER            "user"
#define JSON_OBJECT_RUNTIME         "runtime"
#define JSON_OBJECT_ATTACH_LIST     "attachments"
#define JSON_OBJECT_ATTACHMENT      "attachment"
#define JSON_OBJECT_MSG_TEXT        "message"
#define JSON_OBJECT_TTL             "ttl"
#define JSON_OBJECT_BOR             "bor"
#define JSON_OBJECT_VGROUPID        "vgroupid"

#define JSON_ACTION_SEND_MESSAGE    "sendmessage"
#define JSON_ACTION_ADD_FRIEND      "addfriend"

class WickrBotJson
{
public:
    WickrBotJson();
    ~WickrBotJson();

    bool parseJsonString(QByteArray jsonString);
    int getJsonIntValue(QJsonObject feed, QString key);
    QString getJsonStringValue(QJsonObject object, QString key);
    QStringList getJsonStringList(QJsonObject object, QString attribute, QString key);

    // Defintions of actions
    typedef enum { ACTION_ADD_FRIEND, ACTION_SEND_MESSAGE, } WBJsonActions;

private:
    // Definitions of values parsed from the JSON string
    long ttl;
    long bor;
    bool has_bor;
    WBJsonActions action;
    QList<QString> userIDs;
    QList<QString> userNames;
    QList<QString> attachments;
    QString message;
    QDateTime runTime;
    QString m_vgroupid;

public:
    void setTTL(long ttl) { this->ttl = ttl; }
    void setBOR(long bor) { this->bor = bor; }
    void addAttachment(QString attachment) { this->attachments.append(attachment); }
    void setMessage(QString message) { this->message = message; }
    void setRunTime(QDateTime runTime) { this->runTime = runTime; }

    WBJsonActions getAction() { return action; }
    long getTTL() { return ttl; }
    long getBOR() { return bor; }
    bool hasBOR() { return has_bor; }

    QList<QString> getUserIDs() { return userIDs; }
    QList<QString> getUserNames() { return userNames; }
    QList<QString> getAttachments() { return attachments; }
    QString getMessage() { return message; }
    QString getVGroupID() { return m_vgroupid; }
};

#endif // WICKRBOTJSON_H
