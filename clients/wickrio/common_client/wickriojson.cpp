#include <QString>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDateTime>
#include <QDebug>

#include "wickriojson.h"

WickrBotJson::WickrBotJson() :
    m_vgroupid("")
{

}

WickrBotJson::~WickrBotJson()
{

}

int WickrBotJson::getJsonIntValue(QJsonObject feed, QString key)
{
    QJsonValue value;
    QJsonObject jsonObject;

    if (feed.contains(key)) {
        value = feed[key];
        jsonObject = value.toObject();

        value = jsonObject["$t"];
        if (value.isString()) {
            QString str = value.toString("0");
            return str.toInt();
        } else {
            return value.toInt();
        }
    }

    return 0;
}

QString WickrBotJson::getJsonStringValue(QJsonObject object, QString key)
{
    QJsonValue value;
    QJsonObject jsonObject;

    if (object.contains(key)) {
        value = object[key];
        jsonObject = value.toObject();

        value = jsonObject["$t"];
        if (value.isString()) {
            QString str = value.toString("");
            return str;
        }
    }

    return "";
}

QStringList WickrBotJson::getJsonStringList(QJsonObject object, QString attribute, QString key)
{
    QStringList retList;

    if (object.contains(attribute)) {
        QJsonValue value = object[attribute];
        if (value.isArray()) {
            QJsonArray array = value.toArray();

            for (int i=0; i< array.size(); i++) {
                QJsonValue arrayValue = array[i];
                if (arrayValue.isObject()) {
                    QJsonObject arrayEntry = arrayValue.toObject();
                    if (arrayEntry.contains(key)) {
                        QJsonValue keyValue = arrayEntry[key];
                        if (keyValue.isString())
                            retList += keyValue.toString();
                    }
                }
            }
        }
    }

    return retList;
}

bool
WickrBotJson::parseJsonString(QByteArray jsonString)
{
    QJsonParseError jsonError;
    QJsonDocument jsonResponse = QJsonDocument().fromJson(jsonString, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "parseJsonString:" << jsonError.errorString();
        return false;
    }

    QJsonObject jsonObject = jsonResponse.object();
    QJsonValue value;

    QJsonObject operationObject;

    // Start the operation
    if (jsonObject.contains(JSON_OBJECT_OPERATION)) {
        value = jsonObject[JSON_OBJECT_OPERATION];
        operationObject = value.toObject();
    } else {
        qDebug() << "processContactReply: does not contain operation!";
        return false;
    }

    // Get the Action
    if (operationObject.contains(JSON_OBJECT_ACTION)) {
        value = operationObject[JSON_OBJECT_ACTION];
        QString actionString = value.toString("invalid");
        if (actionString == "invalid") {
            qDebug() << "action is invalid, value is" << actionString;
            return false;
        } else {
            if (actionString == JSON_ACTION_SEND_MESSAGE)
                action = ACTION_SEND_MESSAGE;
            else if (actionString == JSON_ACTION_ADD_FRIEND)
                action = ACTION_ADD_FRIEND;
            else {
                qDebug() << "action is unknown, value is" << actionString;
                return false;
            }
        }
    } else {
        action = ACTION_SEND_MESSAGE;
    }

    QJsonArray entryArray;

    // Parse the contacts
    if (operationObject.contains(JSON_OBJECT_USER_LIST)) {
        value = operationObject[JSON_OBJECT_USER_LIST];
        entryArray = value.toArray();

        for (int i=0; i< entryArray.size(); i++) {
            QJsonValue arrayValue;

            arrayValue = entryArray[i];

            if (arrayValue.isObject()) {
                // Get the title for this contact entry
                QJsonObject arrayObject = arrayValue.toObject();

                if (arrayObject.contains(JSON_OBJECT_USER_ID)) {
                    QJsonValue idobj = arrayObject[JSON_OBJECT_USER_ID];
                    QString id = idobj.toString();
                    userIDs.append(id);
                } else if (arrayObject.contains(JSON_OBJECT_USER_NAME)) {
                    QJsonValue idobj = arrayObject[JSON_OBJECT_USER_NAME];
                    QString name = idobj.toString();
                    userNames.append(name);
                } else {
                    qDebug() << JSON_OBJECT_USER_LIST << "does not contains ID or Name";
                    return false;
                }
            }
        }
    } else if (operationObject.contains(JSON_OBJECT_USER)) {
        value = operationObject[JSON_OBJECT_USER];
        this->userIDs.append(value.toVariant().toString());
    } else if (operationObject.contains(JSON_OBJECT_VGROUPID)) {
        value = operationObject[JSON_OBJECT_VGROUPID];
        this->m_vgroupid = value.toString();
    } else {
        qDebug() << "Does not contain users or vgroupid!";
        return false;
    }

    if (operationObject.contains(JSON_OBJECT_RUNTIME)) {
        value = operationObject[JSON_OBJECT_RUNTIME];

        // Use the current date and time if the one is invalid
        runTime = value.toVariant().toDateTime();
    } else {
        QDateTime dt = QDateTime::currentDateTime();
        runTime = dt;
    }

    // Parse out any attachment
    if (operationObject.contains(JSON_OBJECT_ATTACH_LIST)) {
        value = operationObject[JSON_OBJECT_ATTACH_LIST];
        QJsonArray attachments = value.toArray();
        for (int i=0; i<attachments.size(); i++) {
            this->addAttachment(attachments.at(i).toString());
        }
    } else if (operationObject.contains(JSON_OBJECT_ATTACHMENT)) {
        value = operationObject[JSON_OBJECT_ATTACHMENT];
        this->addAttachment(value.toVariant().toString());
    }

    // Parse out any message
    if (operationObject.contains(JSON_OBJECT_MSG_TEXT)) {
        value = operationObject[JSON_OBJECT_MSG_TEXT];
        message = value.toString();
    }

    // Parse out any TTL
    if (operationObject.contains(JSON_OBJECT_TTL)) {
        value = operationObject[JSON_OBJECT_TTL];
        ttl = value.toInt(0);
    } else {
        ttl = 0;
    }

    return true;
}

