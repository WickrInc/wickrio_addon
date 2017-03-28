#ifndef WICKRBOTJSONDATA_H
#define WICKRBOTJSONDATA_H

#include <QDateTime>

#include "wickrbotlib.h"
#include "operationdata.h"

class DECLSPEC WickrBotJsonData
{
public:
    WickrBotJsonData();
    ~WickrBotJsonData();

    bool parse(OperationData *operation, QByteArray jsonString);
    bool parseSendMessage(OperationData *operation, QByteArray jsonString);

public:
    int m_ttl;
    QString m_action;
    QList<QString> m_userIDs;
    QList<QString> m_userNames;
    QString m_vgroupid;
    QList<QString> m_attachments;
    bool m_rmAttachmentWhenDone;
    QString m_message;
    QDateTime m_runTime;

private:
    OperationData *m_operation;

    bool processJsonDoc(QJsonDocument &jsonResponse);
    bool processSendMessageJsonDoc(const QJsonObject &operationObject);
    bool processLoginJsonDoc(const QJsonObject &operationObject);

    void processAttachment(QJsonObject *object);
    bool processAttachmentURL(QString filename, QString url);
    bool processAttachmentFile(QString filename);


    bool saveAttachment(QString filename, QByteArray data);
    QStringList getJsonStringList(QJsonObject object, QString attribute, QString key);
    QString getJsonStringValue(QJsonObject object, QString key);
    int getJsonIntValue(QJsonObject feed, QString key);
    int processOperationData();
    int processSendMessage();
    int processLoginRequest();

};

#endif // WICKRBOTJSONDATA_H
