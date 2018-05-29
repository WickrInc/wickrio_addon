#include "rabbitmq_iface.h"

using namespace std;

bool
cmdStringGetStatistics(string& command)
{
    command = "{ \"action\" : \"get_statistics\" }";
    return true;
}

bool
cmdStringModifyRoom(string& command, const string& vGroupID)
{
    command = "{ \"action\" : \"modify_room \", \"vgroupid\" : " + vGroupID + " }";
    return true;
}

bool
cmdStringSendMessage(string& command,
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

