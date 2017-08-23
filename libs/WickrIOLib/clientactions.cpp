#include "clientactions.h"

ClientActions::ClientActions(const QString& clientType, OperationData *operation, QObject *parent) : QObject(parent),
    m_clientType(clientType),
    m_operation(operation)
{
}

ClientActions::~ClientActions() {
    reset();
}

void
ClientActions::reset()
{
    for (ClientActionsEntry *client : m_clients) {
        delete client;
    }
    m_clients.clear();
}

void
ClientActions::updateClients()
{
    if (m_operation && m_operation->m_botDB) {
        QList<int> clientIDs = m_operation->m_botDB->getClientIDFromType(m_clientType);
        reset();
        for (int clientID : clientIDs) {
            int count = m_operation->m_botDB->getClientsActionCount(clientID);
            ClientActionsEntry *newEntry = new ClientActionsEntry(clientID, count);
            m_clients.append(newEntry);
        }
    }
}

int
ClientActions::nextClient()
{
    updateClients();

    int lowClient = 0;
    int lowCount = 0xFFFFFF;

    for (ClientActionsEntry *client : m_clients) {
        if (client->m_numQueued < lowCount) {
            lowCount = client->m_numQueued;
            lowClient = client->m_clientID;
        }
    }
    return lowClient;
}

int
ClientActions::getActionCount()
{
    updateClients();
    int count=0;
    for (ClientActionsEntry *client : m_clients) {
        count += client->m_numQueued;
    }
    return count;
}
