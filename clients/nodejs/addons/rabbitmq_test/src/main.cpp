#include "bot_iface.h"

using namespace std;

bool getInput(const string& prompt, string& result, bool allowemptystring=false)
{
    bool done=false;

    while (!done) {
        std::cout << prompt;

        getline(cin, result);

        while (result[result.size() - 1] == '\r' || result[result.size() - 1] == '\n') {
            result.resize(result.size() - 1);
        }

        if (result.size() > 0) {
            if (result == "quit") {
                return false;
            }
            break;
        } else if (allowemptystring) {
            break;
        }
    }

    return true;
}

int main (int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <client name>\n";
        return 1;
    }

    BotIface botIface(argv[1]);
    if (!botIface.init()) {
        std::cout << "Could not initialize Bot Interface!";
        return 1;
    }

    bool done = false;
    while (!done) {
        string input;

        if (!getInput("Enter command: ", input)) {
            done = true;
            continue;
        }

        if (input == "?" || input == "help") {
            std::cout << "Valid commands:\n"
                      << "    get_statistics\n"
                      << "    modify_room\n"
                      << "    send_message\n"
                      << "    quit\n"
                      << "    help\n";
            continue;
        }

        string command;

        if (input == "get_statistics") {
            botIface.cmdStringGetStatistics(command);
        } else if (input == "modify_room") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }

            if (!botIface.cmdStringModifyRoom(command, vGroupID)) {
                std::cout << "Failed to create Modify Room command!";
                continue;
            }
        } else if (input == "send_message") {
            string vGroupID;
            vector <string> users;
            string message;
            string ttl;
            string bor;

            if (!getInput("Enter VGroupID: ", vGroupID, true)) {
                done = true;
                continue;
            }
            if (vGroupID.size() == 0) {
                bool done=false;
                while (!done) {
                    string user;
                    if (!getInput("Enter User: ", user, true)) {
                        done = true;
                        continue;
                    }
                    if (user.size() == 0)
                        break;
                    users.push_back(user);
                }
                if (done)
                    continue;
            }
            if (vGroupID.size() == 0 && users.size() == 0) {
                std::cout << "Must enter either a VGroupID or a list of users!";
                continue;
            }
            if (!getInput("Enter Message to send: ", message, true)) {
                done = true;
                continue;
            }

            if (!botIface.cmdStringSendMessage(command, vGroupID, users, message, ttl, bor)) {
                std::cout << "Failed to create Send Message command!";
                continue;
            }
        } else {
            std::cout << "Unknown command: " << input << "\n";
            continue;
        }

        string response;
        if (! botIface.send(command, response)) {
            std::cout << "Send failed!";
            continue;
        } else {
            if (response.length() > 0) {
                  cout << response << endl;
            }
        }
    }
    return 0;
}
