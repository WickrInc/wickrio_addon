#include "rabbitmq_iface.h"

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

    string clientName = argv[1];

	try {
        RabbitMQIface *iface;

//        string clientName = "bot0525014028@62114373.net";

        iface = new RabbitMQIface(clientName);

        if (!iface->init()) {
            std::cout << "Could not initilize the interface!\n";
        } else {
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
                    command = "{ \"action\" : \"get_statistics\" }";
                } else if (input == "modify_room") {
                    string vGroupID;
                    if (!getInput("Enter VGroupID: ", vGroupID)) {
                        done = true;
                        continue;
                    }
                    command = "{ \"action\" : \"modify_room \", \"vgroupid\" : " + vGroupID + " }";
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
                } else {
                    std::cout << "Unknown command: " << input << "\n";
                    continue;
                }

                string response = iface->sendMessage(command);

                if (response.length() > 0) {
                      cout << response << endl;
                }
            }
        }
    } catch (AMQPException e) {
        std::cout << "Got Exception:" << e.getMessage() << std::endl;
	}

	return 0;

}
