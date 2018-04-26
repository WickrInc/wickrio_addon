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

    bool addRoom(const QByteArray& json, QString& responseString);
    bool updateRoom(const QString &vGroupID, const QByteArray& json, QString& responseString);

    bool addGroupConvo(const QByteArray& json, QString& responseString);

private:
    OperationData *m_operation;

    QStringList getJsonArrayValue(QJsonObject jsonObject, QString jsonName, QString jsonArray);
    bool updateAndValidateMembers(QString& responseString, const QStringList& memberslist, QStringList *memberHashes=nullptr);

signals:
    void signalMemberSearchDone();

};

#endif // WICKRIOAPIINTERFACE_H
