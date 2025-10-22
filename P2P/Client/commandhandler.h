#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <string>
#include <vector>
#include <unordered_set>

using namespace std;

class CommandHandler {
private:
    // Helper methods
    string extract_command_name(const string& command);
    bool check_login_required(const string& command_name, int check_login);
    bool is_valid_command(const string& command_name, const unordered_set<string>& valid_commands);

public:
    void run_client(const string& ip, int port);
};

#endif // COMMANDHANDLER_H