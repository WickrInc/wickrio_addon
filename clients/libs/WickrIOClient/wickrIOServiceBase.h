#ifndef WICKRIOSERVICEBASE_H
#define WICKRIOSERVICEBASE_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include "common/wickrNetworkUtil.h"
#include "operationdata.h"

class WickrIOServiceBase : public QObject
{
    Q_OBJECT
public:
    explicit WickrIOServiceBase(const QString& serviceName);
    virtual ~WickrIOServiceBase();

    QString serviceName() { return m_serviceName; }

protected:
    WickrServiceState   m_state;        // state of the service
    QThread             m_thread;       // thread associated with the service
    long                m_heartbeat;    // heartbeat of the service, for keep alives

    void startHeartBeat();
    void stopHeartBeat();

private:
    QString             m_serviceName;  // Unique name of this service
    QTimer              m_timer;

private slots:
    void slotTimerExpire();

signals:
    void signalHeartBeat(WickrIOServiceBase *svc);

};

#endif // WICKRIOSERVICEBASE_H
