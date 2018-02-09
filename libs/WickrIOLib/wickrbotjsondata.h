#ifndef WICKRBOTJSONDATA_H
#define WICKRBOTJSONDATA_H

#include <QDateTime>

#include "wickrbotlib.h"
#include "operationdata.h"
#include "clientactions.h"

class DECLSPEC WickrBotJsonData
{
public:
    WickrBotJsonData(OperationData *operation);
    ~WickrBotJsonData();

    bool parse(QByteArray jsonString);
    bool parseJson4SendMessage(QByteArray jsonString);
    bool postEntry4SendMessage();
    bool parseSendMessage(QByteArray jsonString);

    void setClientType(const QString &clientType);

    QStringList getUserNames() { return m_userNames; }
    QString getVGroupID() { return m_vgroupid; }

    void clearLastError() { m_lastError.clear(); }
    void setLastError(const QString errstr) { m_lastError = errstr; }
    QString getLastError() { return m_lastError; }

public:
    int m_ttl;
    int m_bor;
    bool m_has_bor;
    QString m_action;
    QStringList m_userIDs;
    QStringList m_userNames;
    QString m_vgroupid;
    QStringList m_attachments;
    bool m_rmAttachmentWhenDone;
    QString m_message;
    QDateTime m_runTime;
    QString m_statususer;

    QString m_lastError;

private:
    OperationData   *m_operation;
    ClientActions   *m_clientActions;

    bool processJsonDoc(QJsonDocument &jsonResponse);
    bool processSendMessageJsonDoc(const QJsonObject &operationObject);
    bool processSendMessageJsonDocV3(const QJsonObject &operationObject);
    bool processLoginJsonDoc(const QJsonObject &operationObject);

    bool processAttachments(const QJsonObject &operationObject);

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
    int getClientID();

};

#endif // WICKRBOTJSONDATA_H
