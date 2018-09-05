#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include "bot_iface.h"

using namespace std;

BotIface *pBotIface;

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

bool getList(const string& prompt, vector <string>& result, bool allowemptylist=false)
{
    bool done=false;
    while (!done) {
        string input;
        if (!getInput(prompt, input, true)) {
            return false;
        }
        if (input.size() == 0) {
            if (result.size() == 0 && !allowemptylist) {
                cout << "ERROR: Must enter at least one value!";
                continue;
            }
            break;
        }
        result.push_back(input);
    }

    return true;
}

void asyncMessage(string message)
{
    std::cout << "Got message\n";
}

void asyncEvent(string event)
{
    std::cout << "Got event\n";
}

void mainThread()
{
    bool done = false;
    while (!done) {
        string input;

        if (!getInput("Enter command: ", input)) {
            done = true;
            continue;
        }

        if (input == "?" || input == "help") {
            std::cout << "Secure Room commands:\n"
                      << "    add_room\n"
                      << "    delete_room\n"
                      << "    get_room\n"
                      << "    get_rooms\n"
                      << "    leave_room\n"
                      << "    modify_room\n"
                      << "\nGroup Conversation commands:\n"
                      << "    add_groupconvo\n"
                      << "    get_groupconvo\n"
                      << "    get_groupconvos\n"
                      << "    delete_groupconvo\n"
                      << "\nStatistics commands:\n"
                      << "    clear_statistics\n"
                      << "    get_statistics\n"
                      << "\nMessage commands:\n"
                      << "    get_message\n"
                      << "    send_message\n"
                      << "    send_file\n"
                      << "\nAsync messages:\n"
                      << "    start_async_messages\n"
                      << "    stop_async_messages\n"
                      << "    start_async_events\n"
                      << "    stop_async_events\n"
                      << "\nMisc. commands:\n"
                      << "    quit\n"
                      << "    help\n";
            continue;
        }

        string command;

        if (input == "clear_statistics") {
            pBotIface->cmdStringClearStatistics(command);
        } else if (input == "get_statistics") {
            pBotIface->cmdStringGetStatistics(command);
        } else if (input == "add_room") {
            vector <string> members;
            vector <string> moderators;
            string title;
            string description;
            string ttl;
            string bor;

            if (!getInput("Enter Title: ", title, false) ||
                !getInput("Enter Description: ", description, true) ||
                !getInput("Enter TTL: ", ttl, true) ||
                !getInput("Enter BOR: ", bor, true) ||
                !getList("Enter Member: ", members, false) ||
                !getList("Enter Moderator: ", moderators, false)) {
                done = true;
                continue;
            }

            if (pBotIface->cmdStringAddRoom(command, members, moderators, title, description, ttl, bor) != BotIface::SUCCESS) {
                std::cout << "Failed to create Add Room command!";
                continue;
            }
        } else if (input == "modify_room") {
            string vGroupID;
            vector <string> members;
            vector <string> moderators;
            string title;
            string description;
            string ttl;
            string bor;

            if (!getInput("Enter VGroupID: ", vGroupID) ||
                !getInput("Enter Title: ", title, true) ||
                !getInput("Enter Description: ", description, true) ||
                !getInput("Enter TTL: ", ttl, true) ||
                !getInput("Enter BOR: ", bor, true) ||
                !getList("Enter Member: ", members, true) ||
                !getList("Enter Moderator: ", moderators, true)) {
                done = true;
                continue;
            }

            if (pBotIface->cmdStringModifyRoom(command, vGroupID, members, moderators, title, description, ttl, bor) != BotIface::SUCCESS) {
                std::cout << "Failed to create Modify Room command!";
                continue;
            }
        } else if (input == "get_room") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }
            if (pBotIface->cmdStringGetRoom(command, vGroupID) != BotIface::SUCCESS) {
                std::cout << "Failed to create Get Room command!";
                continue;
            }
        } else if (input == "get_rooms") {
                pBotIface->cmdStringGetRooms(command);
        } else if (input == "delete_room") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }
            if (pBotIface->cmdStringDeleteRoom(command, vGroupID) != BotIface::SUCCESS) {
                std::cout << "Failed to create Delete Room command!";
                continue;
            }
        } else if (input == "leave_room") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }
            if (pBotIface->cmdStringLeaveRoom(command, vGroupID) != BotIface::SUCCESS) {
                std::cout << "Failed to create Leave Room command!";
                continue;
            }
        } else if (input == "add_groupconvo") {
            vector <string> members;
            string ttl;
            string bor;

            if (!getInput("Enter TTL: ", ttl, true) ||
                !getInput("Enter BOR: ", bor, true) ||
                !getList("Enter Member: ", members, false)) {
                done = true;
                continue;
            }

            if (pBotIface->cmdStringAddGroupConvo(command, members, ttl, bor) != BotIface::SUCCESS) {
                std::cout << "Failed to create Add Group Conversation command!";
                continue;
            }
        } else if (input == "get_groupconvo") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }
            if (pBotIface->cmdStringGetGroupConvo(command, vGroupID) != BotIface::SUCCESS) {
                std::cout << "Failed to create Get Group Conversaion command!";
                continue;
            }
        } else if (input == "get_groupconvos") {
                pBotIface->cmdStringGetGroupConvos(command);
        } else if (input == "delete_groupconvo") {
            string vGroupID;
            if (!getInput("Enter VGroupID: ", vGroupID)) {
                done = true;
                continue;
            }
            if (pBotIface->cmdStringDeleteGroupConvo(command, vGroupID) != BotIface::SUCCESS) {
                std::cout << "Failed to create Delete Group Conversaion command!";
                continue;
            }
        } else if (input == "get_message") {
                pBotIface->cmdStringGetReceivedMessage(command);
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

            if (pBotIface->cmdStringSendMessage(command, vGroupID, users, message, ttl, bor) != BotIface::SUCCESS) {
                std::cout << "Failed to create Send Message command!";
                continue;
            }
        } else if (input == "send_file") {
            string vGroupID;
            vector <string> users;
            string filename;
            string displayname;
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
            if (!getInput("Enter filename to send: ", filename, true)) {
                done = true;
                continue;
            }

            // Allocate the destination space
            std::string filenameLower;
            filenameLower.resize(filename.size());

            // Convert the source string to lower case
            // storing the result in destination string
            std::transform(filename.begin(),
                           filename.end(),
                           filenameLower.begin(),
                           ::tolower);
            if (filenameLower.find("http") == 0) {
                if (!getInput("Enter file's display name: ", displayname, true)) {
                    done = true;
                    continue;
                }
            }

            if (pBotIface->cmdStringSendAttachment(command, vGroupID, users, filename, displayname, ttl, bor) != BotIface::SUCCESS) {
                std::cout << "Failed to create Send Message command!";
                continue;
            }
        } else if (input == "start_async_messages") {
            pBotIface->cmdStringStartAsyncRecvMessages(command, asyncMessage);
        } else if (input == "stop_async_messages") {
            pBotIface->cmdStringStopAsyncRecvMessages(command);
        } else if (input == "start_async_events") {
            pBotIface->cmdStringStartAsyncRecvEvents(command, asyncEvent);
        } else if (input == "stop_async_events") {
            pBotIface->cmdStringStopAsyncRecvEvents(command);
        } else {
            std::cout << "Unknown command: " << input << "\n";
            continue;
        }

        string response;
        if (pBotIface->send(command, response) != BotIface::SUCCESS) {
            std::cout << "Send failed: " << pBotIface->getLastErrorString();
            continue;
        } else {
            if (response.length() > 0) {
                  cout << response << endl;
            }
        }
    }
}

int main (int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <client name>\n";
        return 1;
    }

    BotIface botIface(argv[1]);
    if (botIface.init() != BotIface::SUCCESS) {
        std::cout << "Could not initialize Bot Interface!";
        std::cout << botIface.getLastErrorString();
        return 1;
    }

    pBotIface = &botIface;

    thread mnThread(mainThread);

    mnThread.join();

    return 0;
}
