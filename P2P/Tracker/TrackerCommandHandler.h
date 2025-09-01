#ifndef TRACKER_COMMAND_HANDLER_H
#define TRACKER_COMMAND_HANDLER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_set>

using namespace std;

struct FileInfo {
    string fileName;
    string groupId;
    string clientId;
    string port;
    vector<string> chunkSHA1s;
    bool isLoggedIn;
    bool stop_share;
};

class TrackerCommandHandler {
public:
    // Constructor
    TrackerCommandHandler();

    // Client command handlers
    string handle_create_user(const vector<string>& tokenise_commands, int client_socket);
    string handle_login(const vector<string>& tokenise_commands, int client_socket);
    string handle_create_group(const vector<string>& tokenise_commands, int client_socket);
    string handle_join_group(const vector<string>& tokenise_commands, int client_socket);
    string handle_list_requests(const vector<string>& tokenise_commands, int client_socket);
    string handle_list_groups(const vector<string>& tokenise_commands, int client_socket);
    string handle_accept_request(const vector<string>& tokenise_commands, int client_socket);
    string handle_leave_group(const vector<string>& tokenise_commands, int client_socket);
    string handle_logout(const vector<string>& tokenise_commands, int client_socket);
    string handle_upload_file(const vector<string>& tokenise_commands, int client_socket);
    string handle_download_file(const vector<string>& tokenise_commands, int client_socket);
    string handle_stop_share(const vector<string>& tokenise_commands, int client_socket);

    // Getters for external access to data structures
    map<string, string>& getUsers();
    map<string, set<string>>& getGroups();
    map<string, string>& getGroupOwner();
    map<string, set<string>>& getJoiningRequest();
    map<string, int>& getOneLogin();
    map<pair<string, int>, unordered_set<int>>& getChunkownerPorts();
    vector<FileInfo>& getTrackerFiles();
    vector<FileInfo>& getTotalClients();
    string& getLogin();
    vector<int>& getGroupList();

private:
    // Data structures moved from global scope
    map<string, string> users;
    map<string, set<string>> groups;
    map<string, string> group_owner;
    map<string, set<string>> joining_request;
    map<string, int> one_login;
    map<pair<string, int>, unordered_set<int>> chunkownerPorts;
    vector<FileInfo> trackerFiles;
    vector<FileInfo> totalclients;
    string login;
    vector<int> group_list;
};

#endif // TRACKER_COMMAND_HANDLER_H
