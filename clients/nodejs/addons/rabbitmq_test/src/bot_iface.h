using namespace std;

bool cmdStringGetStatistics(string& command);
bool cmdStringModifyRoom(string& command, const string& vGroupID);
bool cmdStringSendMessage(string& command,
                          const string& vGroupID,
                          const vector <string>& users,
                          const string& message,
                          const string& ttl,
                          const string& bor);
