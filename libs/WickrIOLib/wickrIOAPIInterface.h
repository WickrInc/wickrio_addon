#ifndef WICKRIOAPIINTERFACE_H
#define WICKRIOAPIINTERFACE_H

#include <QObject>
#include "operationdata.h"

#include "messaging/wickrConvo.h"

class WickrIOAPIInterface : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(WickrIOAPIInterface)

public:
    WickrIOAPIInterface(OperationData *operation, QObject* parent=0);

    bool sendMessage(const QByteArray& json, QString& responseString);
    bool getReceivedMessages(QString& responseString);

    bool addRoom(const QByteArray& json, QString& responseString);
    bool updateRoom(const QString &vGroupID, const QByteArray& json, QString& responseString);
    bool deleteRoom(const QString &vGroupID, QString& responseString);
    bool leaveRoom(const QString &vGroupID, QString& responseString);
    bool getRoom(const QString &vGroupID, QString& responseString);
    bool getRooms(QString& responseString);

    bool addGroupConvo(const QByteArray& json, QString& responseString);
    bool deleteGroupConvo(const QString &vGroupID, QString& responseString);
    bool getGroupConvo(const QString &vGroupID, QString& responseString);
    bool getGroupConvos(QString& responseString);

    bool getStatistics(const QString& apiKey, QString& responseString);
    bool clearStatistics(const QString& apiKey, QString& responseString);

    bool startAsyncMessages(QString& responseString);
    bool stopAsyncMessages(QString& responseString);
    bool startAsyncEvents(QString& responseString);
    bool stopAsyncEvents(QString& responseString);

    bool processAsyncMessages() { return m_processAsyncMessages; }
    bool processAsyncEvents() { return m_processAsyncEvents; }

private:
    OperationData *m_operation;
    bool    m_processAsyncMessages = false;
    bool    m_processAsyncEvents = false;

    bool deleteConvo(bool isSecureConvo, const QString& vgroupID);
    QJsonObject getRoomInfo(WickrCore::WickrConvo *convo);
    QJsonObject getGroupConvoInfo(WickrCore::WickrConvo *convo);

    QStringList getJsonArrayValue(QJsonObject jsonObject, QString jsonName, QString jsonArray);
    bool updateAndValidateMembers(QString& responseString, const QStringList& memberslist, QStringList *memberHashes=nullptr);
    bool updateAndUserIDs(QString& responseString, const QStringList& userIDs);

    int numMessages();

signals:
    void signalMemberSearchDone();

};

#endif // WICKRIOAPIINTERFACE_H
