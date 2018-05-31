#ifndef BOT_IFACE_H
#define BOT_IFACE_H

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif

#include "rabbitmq_iface.h"

#include <unistd.h>
#include <vector>
#include <iostream>
#include <v8.h>

using namespace v8;
using namespace std;

class BotIface
{
public:
    BotIface(const string& client);
    ~BotIface();

    enum BotIfaceStatus {
        SUCCESS = 0,

        INIT_FAILED,
        QEXCEPTION,

        MISSING_INPUT_FIELD,
        INVALID_FIELD_VALUE,
    };

    BotIfaceStatus init();
    BotIfaceStatus send(const string& command, string& reponse);
    void display(string & a);
    BotIfaceStatus cmdStringGetStatistics(string& command);
    BotIfaceStatus cmdStringClearStatistics(string& command);

    BotIfaceStatus cmdStringGetRooms(string& command);
    BotIfaceStatus cmdStringAddRoom(string& command,
                                    const vector <string>& members,
                                    const vector <string>& moderators,
                                    const string& title,
                                    const string& description,
                                    const string& ttl,
                                    const string& bor);
    BotIfaceStatus cmdStringModifyRoom(string& command,
                                       const string& vGroupID,
                                       const vector <string>& members,
                                       const vector <string>& moderators,
                                       const string& title,
                                       const string& description,
                                       const string& ttl,
                                       const string& bor);
    BotIfaceStatus cmdStringGetRoom(string& command, const string& vGroupID);
    BotIfaceStatus cmdStringLeaveRoom(string& command, const string& vGroupID);
    BotIfaceStatus cmdStringDeleteRoom(string& command, const string& vGroupID);

    BotIfaceStatus cmdStringAddGroupConvo(string& command,
                                          const vector <string>& members,
                                          const string& ttl,
                                          const string& bor);
    BotIfaceStatus cmdStringDeleteGroupConvo(string& command, const string& vGroupID);
    BotIfaceStatus cmdStringGetGroupConvo(string& command, const string& vGroupID);
    BotIfaceStatus cmdStringGetGroupConvos(string& command);

    BotIfaceStatus cmdStringGetReceivedMessage(string& command);
    BotIfaceStatus cmdStringSendMessage(string& command,
                                        const string& vGroupID,
                                        const vector <string>& users,
                                        const string& message,
                                        const string& ttl,
                                        const string& bor);


    string getLastErrorString() { return m_lastError; }
    void clearLastError() { m_lastError = ""; }

private:
    string          m_clientName;
    RabbitMQIface   *m_iface = nullptr;

    string          m_lastError = "";

    BotIfaceStatus setRoomFields(string& command,
                                 const vector <string>& members,
                                 const vector <string>& moderators,
                                 const string& title,
                                 const string& description,
                                 const string& ttl,
                                 const string& bor);

    // Utility functions
    bool is_digits(const string &str);
    void addUserList(const vector <string>& list, string& tostring);

};

#endif // BOT_IFACE_H
