#include <string>
#include <algorithm>

#include "bot_iface.h"
#include <iostream>

using namespace std;

BotIface::BotIface(const string& client)
{
    string m_clientName = client;
    int pos;

    while ((pos = m_clientName.find('@')) != std::string::npos)
        m_clientName.replace(pos, 1, "_");

    m_iface = new MesgQueueIface(m_clientName);
}

BotIface::~BotIface()
{
    if (m_iface != nullptr) {
        delete m_iface;
    }
}

BotIface::BotIfaceStatus
BotIface::init()
{
    if (!m_iface->init()) {
        m_lastError = "Could not initialize the interface!";
        return INIT_FAILED;
    }

    return SUCCESS;
}

/**
 * @brief BotIface::send
 * This function will send the input command
 * @param command
 * @return
 */
BotIface::BotIfaceStatus
BotIface::send(const string& command, string& response)
{
    response = m_iface->sendMessage(command);
    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BotIface::BotIfaceStatus
BotIface::setRoomFields(string& command,
                        const vector <string>& members,
                        const vector <string>& moderators,
                        const string& title,
                        const string& description,
                        const string& ttl,
                        const string& bor)
{
    int numVals = ((title.length()>0) ? 1 : 0) +
                  ((description.length()>0) ? 1 : 0) +
                  ((ttl.length()>0) ? 1 : 0) +
                  ((bor.length()>0) ? 1 : 0) +
                  ((members.size()>0) ? 1 : 0) +
                  ((moderators.size()>0) ? 1 : 0);

    if (title.length() > 0) {
        command += "\"title\" : \"" + title + "\"";
        if (--numVals)
            command += ", ";
    }

    // TODO: Need to deal with special characters!
    if (description.size() > 0) {
        command += " \"description\" : \"" + description + "\"";
        if (--numVals)
            command += ", ";
    }

    if (ttl.size() > 0) {
        if (!is_digits(ttl)) {
            m_lastError = "AddRoom: TTL must be a number";
            return INVALID_FIELD_VALUE;
        }
        command += " \"ttl\" : " + ttl + "";
        if (--numVals)
            command += ", ";
    }
    if (bor.size() > 0) {
        if (!is_digits(bor)) {
            m_lastError = "AddRoom: BOR must be a number";
            return INVALID_FIELD_VALUE;
        }
        command += " \"bor\" : " + bor + "";
        if (--numVals)
            command += ", ";
    }

    if (members.size() > 0) {
        command += "\"members\" : ";
        addUserList(members, command);
        if (--numVals)
            command += ", ";
    }
    if (moderators.size() > 0) {
        command += "\"masters\" : ";
        addUserList(moderators, command);
    }

    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringAddRoom(string& command,
                           const vector <string>& members,
                           const vector <string>& moderators,
                           const string& title,
                           const string& description,
                           const string& ttl,
                           const string& bor)
{
    if (title.length() == 0) {
        m_lastError = "AddRoom: Title must be set!";
        return MISSING_INPUT_FIELD;
    }
    if (members.size() == 0) {
        m_lastError = "AddRoom: Must be at least one member of new room!";
        return MISSING_INPUT_FIELD;
    }
    if (moderators.size() == 0) {
        m_lastError = "AddRoom: Must be at least one moderator of new room!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"add_room\", \"room\" : { ";
    BotIfaceStatus status = setRoomFields(command, members, moderators, title, description, ttl, bor);
    command += "} }";

    return status;
}

BotIface::BotIfaceStatus
BotIface::cmdStringModifyRoom(string& command,
                              const string& vGroupID,
                              const vector <string>& members,
                              const vector <string>& moderators,
                              const string& title,
                              const string& description,
                              const string& ttl,
                              const string& bor)
{
    if (vGroupID.length() == 0) {
        m_lastError = "ModifyRoom: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"modify_room\", \"vgroupid\" : \"" + vGroupID + "\", ";
    BotIfaceStatus status = setRoomFields(command, members, moderators, title, description, ttl, bor);
    command += " }";

    return status;
}

BotIface::BotIfaceStatus
BotIface::cmdStringDeleteRoom(string& command, const string& vGroupID)
{
    if (vGroupID.length() == 0) {
        m_lastError = "DeleteRoom: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"delete_room\", \"vgroupid\" : \"" + vGroupID + "\" }";

    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringLeaveRoom(string& command, const string& vGroupID)
{
    if (vGroupID.length() == 0) {
        m_lastError = "LeaveRoom: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"leave_room\", \"vgroupid\" : \"" + vGroupID + "\" }";

    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringGetRooms(string& command)
{
    command = "{ \"action\" : \"get_rooms\" }";
    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringGetRoom(string& command, const string& vGroupID)
{
    if (vGroupID.length() == 0) {
        m_lastError = "GetRoom: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"get_room\", \"vgroupid\" : \"" + vGroupID + "\" }";

    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BotIface::BotIfaceStatus
BotIface::cmdStringAddGroupConvo(string& command,
                                 const vector <string>& members,
                                 const string& ttl,
                                 const string& bor)
{
    vector <string> emptylist;
    string emptystring;

    if (members.size() == 0) {
        m_lastError = "AddGroupConvo: Must be at least one member of new room!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"add_groupconvo\", \"groupconvo\" : { ";
    BotIfaceStatus status = setRoomFields(command, members, emptylist, emptystring, emptystring, ttl, bor);
    command += "} }";

    return status;
}

BotIface::BotIfaceStatus
BotIface::cmdStringDeleteGroupConvo(string& command, const string& vGroupID)
{
    if (vGroupID.length() == 0) {
        m_lastError = "DeleteGroupConvo: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"delete_groupconvo\", \"vgroupid\" : \"" + vGroupID + "\" }";

    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringGetGroupConvo(string& command, const string& vGroupID)
{
    if (vGroupID.length() == 0) {
        m_lastError = "GetGroupConvo: VGroupID must be set!";
        return MISSING_INPUT_FIELD;
    }

    command = "{ \"action\" : \"get_groupconvo\", \"vgroupid\" : \"" + vGroupID + "\" }";

    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringGetGroupConvos(string& command)
{
    command = "{ \"action\" : \"get_groupconvos\" }";
    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief BotIface::cmdStringGetStatistics
 * Get the current statistics on the client
 * @param command
 * @return
 */
BotIface::BotIfaceStatus
BotIface::cmdStringGetStatistics(string& command)
{
    command = "{ \"action\" : \"get_statistics\" }";
    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringClearStatistics(string& command)
{
    command = "{ \"action\" : \"clear_statistics\" }";
    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BotIface::BotIfaceStatus
BotIface::cmdStringGetReceivedMessage(string& command)
{
    command = "{ \"action\" : \"get_received_messages\" }";
    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringSendMessage(string& command,
                     const string& vGroupID,
                     const vector <string>& users,
                     const string& message,
                     const string& ttl,
                     const string& bor)
{
    string optionalFields = "";

    if (vGroupID.size() == 0 && users.size() == 0) {
        m_lastError = "SendMessage: Must enter either a VGroupID or a list of users!";
        return MISSING_INPUT_FIELD;
    }
    if (ttl.size() > 0) {
        if (!is_digits(ttl)) {
            m_lastError = "SendMessage: TTL must be a number";
            return INVALID_FIELD_VALUE;
        }
        optionalFields += " \"ttl\" : " + ttl + ", ";
    }
    if (bor.size() > 0) {
        if (!is_digits(bor)) {
            m_lastError = "SendMessage: BOR must be a number";
            return INVALID_FIELD_VALUE;
        }
        optionalFields += " \"bor\" : " + bor + ", ";
    }

    if (vGroupID.size() > 0) {
        command = "{ \"action\" : \"send_message\", \"vgroupid\" : \"" + vGroupID + "\" , " \
                + optionalFields
                + "\"message\" : \"" + message \
                + "\" }";
    } else {
        command = "{ \"action\" : \"send_message\", \"message\" : \"" + message + "\" , "
                + optionalFields
                + "\"users\" : ";
        addUserList(users, command);
        command += " }";
    }
    return SUCCESS;
}

BotIface::BotIfaceStatus
BotIface::cmdStringSendAttachment(string& command,
                                  const string& vGroupID,
                                  const vector <string>& users,
                                  const string& attachment,
                                  const string& displayname,
                                  const string& ttl,
                                  const string& bor)
{
    string optionalFields = "";
    if (vGroupID.size() == 0 && users.size() == 0) {
        m_lastError = "SendAttachment: Must enter either a VGroupID or a list of users!";
        return MISSING_INPUT_FIELD;
    }
    if (ttl.size() > 0) {
        if (!is_digits(ttl)) {
            m_lastError = "SendAttachment: TTL must be a number";
            return INVALID_FIELD_VALUE;
        }
        optionalFields += " \"ttl\" : " + ttl + ", ";
    }
    if (bor.size() > 0) {
        if (!is_digits(bor)) {
            m_lastError = "SendAttachment: BOR must be a number";
            return INVALID_FIELD_VALUE;
        }
        optionalFields += " \"bor\" : " + bor + ", ";
    }

    // Calculate the attachment contents, determine if this is a URL or not
    string attachmentJSON;
    string attachmentLower;
    attachmentLower.resize(attachment.size());

    // Convert the source string to lower case
    // storing the result in destination string
    std::transform(attachment.begin(),
                   attachment.end(),
                   attachmentLower.begin(),
                   ::tolower);
    if (attachmentLower.find("http") == 0) {
        attachmentJSON = string("{\"url\" : \"" + attachment + "\", \"filename\" : \"" + displayname + "\" }");
    } else {
        attachmentJSON = string("{\"filename\" : \"" + attachment + "\" }");
    }

    // create the JSON to send to the client
    if (vGroupID.size() > 0) {
        command = "{ \"action\" : \"send_message\", \"attachment\" : " + attachmentJSON + " , "
                + optionalFields
                + "\"vgroupid\" : \"" + vGroupID \
                + "\" }";
    } else {
        command = "{ \"action\" : \"send_message\", \"attachment\" : " + attachmentJSON + " , "
                + optionalFields
                + "\"users\" : ";
        addUserList(users, command);
        command += " }";
    }
    return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

bool
BotIface::is_digits(const string &str)
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}

/**
 * @brief BotIface::addUserList
 * This function will add the JSON list of the input list to the input string
 * @param list
 * @param tostring
 */
void
BotIface::addUserList(const vector <string>& list, string& tostring)
{
    tostring += "[ ";
    for (int i=0; i<list.size(); i++) {
        tostring += "{ \"name\" : \"" + list.at(i) + "\" }";
        if (i < (list.size() - 1)) {
            tostring += ",";
        }
    }
    tostring += " ]";
}
