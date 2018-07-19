#ifndef BOT_IFACE_H
#define BOT_IFACE_H

#if __cplusplus > 199711L // C++11 or greater
#include <functional>
#endif

#include <vector>

#include "zeromq_iface.h"

using namespace std;

class BotIface
{
public:
    BotIface(const string& client);
    ~BotIface();

    /*
     * Definition of return values from class functions
     */
    enum BotIfaceStatus {
        SUCCESS = 0,

        INIT_FAILED,
        QEXCEPTION,

        MISSING_INPUT_FIELD,
        INVALID_FIELD_VALUE,
    };

    /*
     * Initialization function.  Must be called before starting
     */
    BotIfaceStatus init();

    /*
     * Function to send a request to the client. Response is the value
     * returned from the client.
     */
    BotIfaceStatus send(const string& command, string& reponse);

    /*
     * These functions are to access the error information associated
     * with the last error that occurred
     */
    string getLastErrorString() { return m_lastError; }
    void clearLastError() { m_lastError = ""; }


    /*
     * Definition of functions that create strings to be sent to client
     */

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
                                        const string& ttl="",
                                        const string& bor="");
    BotIfaceStatus cmdStringSendAttachment(string& command,
                                           const string& vGroupID,
                                           const vector <string>& users,
                                           const string& attachment,
                                           const string& displayname,
                                           const string& ttl="",
                                           const string& bor="");

private:
    MesgQueueIface  *m_iface = NULL;

    string          m_clientName;           // Name of the client interfacing with
    string          m_lastError = "";       // Last error string

    // Generic function to set the fields associated with an add/modify room command
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