#include "bot_iface.h"

using namespace std;

BotIface::BotIface(const string& client)
{
    string m_clientName = client;
    m_iface = new RabbitMQIface(m_clientName);
}

BotIface::~BotIface()
{
    if (m_iface != nullptr) {
        delete m_iface;
    }
}

bool
BotIface::init()
{
    try {
        if (!m_iface->init()) {
            std::cout << "Could not initilize the interface!\n";
            return false;
        }
    } catch (AMQPException e) {
        std::cout << "Init got exception:" << e.getMessage() << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief BotIface::send
 * This function will send the input command
 * @param command
 * @return
 */
bool
BotIface::send(const string& command, string& response)
{
    try {
        response = m_iface->sendMessage(command);
    } catch (AMQPException e) {
        std::cout << "Send got exception:" << e.getMessage() << std::endl;
        return false;
    }
    return true;
}

bool
BotIface::cmdStringGetStatistics(string& command)
{
    command = "{ \"action\" : \"get_statistics\" }";
    return true;
}

bool
BotIface::cmdStringModifyRoom(string& command, const string& vGroupID)
{
    command = "{ \"action\" : \"modify_room \", \"vgroupid\" : " + vGroupID + " }";
    return true;
}

bool
BotIface::cmdStringSendMessage(string& command,
                     const string& vGroupID,
                     const vector <string>& users,
                     const string& message,
                     const string& ttl,
                     const string& bor)
{
    if (vGroupID.size() == 0 && users.size() == 0) {
        std::cout << "Must enter either a VGroupID or a list of users!";
        return false;
    }
    if (vGroupID.size() > 0) {
        command = "{ \"action\" : \"send_message\", \"vgroupid\" : " + vGroupID + ", " \
                + "\"message\" : \"" + message \
                + "\" }";
    } else {
        command = "{ \"action\" : \"send_message\", \"message\" : \"" + message + "\" , "
                + "\"users\" : [ ";
        for (int i=0; i<users.size(); i++) {
            command += "{ \"name\" : \"" + users.at(0) + "\" }";
            if (i < (users.size() - 1)) {
                command += ",";
            }
        }
        command += " ] }";
    }
    return true;
}

