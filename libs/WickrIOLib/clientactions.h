#ifndef CLIENTACTIONS_H
#define CLIENTACTIONS_H

#include <QObject>

#include "wickrbotlib.h"
#include "operationdata.h"

class ClientActionsEntry
{
public:
    ClientActionsEntry(int clientid) :
        m_clientID(clientid),
        m_numQueued(0)
    {}
    ClientActionsEntry(int clientid, int numQueued) :
        m_clientID(clientid),
        m_numQueued(numQueued)
    {}

    int m_clientID;
    int m_numQueued;
};


class ClientActions : public QObject
{
    Q_OBJECT
public:
    explicit ClientActions(const QString& clientType, OperationData *operation, QObject *parent = nullptr);
    ~ClientActions();

    int nextClient();
    int getActionCount();

private:
    QString m_clientType;
    OperationData *m_operation;
    QList<ClientActionsEntry *>  m_clients;

    void updateClients();
    void reset();

signals:

public slots:
};

#endif // CLIENTACTIONS_H
