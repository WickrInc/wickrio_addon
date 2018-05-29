#ifndef BOT_IFACE_H
#define BOT_IFACE_H

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif

#include "rabbitmq_iface.h"

using namespace std;

class BotIface
{
public:
    BotIface(const string& client);
    ~BotIface();

    bool init();
    bool send(const string& command, string& reponse);

    bool cmdStringGetStatistics(string& command);
    bool cmdStringModifyRoom(string& command, const string& vGroupID);
    bool cmdStringSendMessage(string& command,
                              const string& vGroupID,
                              const vector <string>& users,
                              const string& message,
                              const string& ttl,
                              const string& bor);

private:
    string          m_clientName;
    RabbitMQIface   *m_iface;

};

#endif // BOT_IFACE_H
