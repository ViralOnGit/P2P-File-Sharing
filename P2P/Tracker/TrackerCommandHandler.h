#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include "utils.h"
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

class TrackerCommandHandler;

class BaseCommandHandler {
protected:
    TrackerCommandHandler* tracker;

public:
    BaseCommandHandler(TrackerCommandHandler* tracker);
};

class UserCommandHandler : public BaseCommandHandler {
public:
    UserCommandHandler(TrackerCommandHandler* tracker);
    string handle_create_user(const vector<string>& tokenise_commands, int client_socket);
    string handle_login(const vector<string>& tokenise_commands, int client_socket);
    string handle_logout(const vector<string>& tokenise_commands, int client_socket);
};

class GroupCommandHandler : public BaseCommandHandler {
public:
    GroupCommandHandler(TrackerCommandHandler* tracker);
    string handle_create_group(const vector<string>& tokenise_commands, int client_socket);
    string handle_join_group(const vector<string>& tokenise_commands, int client_socket);
    string handle_list_groups(const vector<string>& tokenise_commands, int client_socket);
    string handle_leave_group(const vector<string>& tokenise_commands, int client_socket);
};

class RequestCommandHandler : public BaseCommandHandler {
public:
    RequestCommandHandler(TrackerCommandHandler* tracker);
    string handle_list_requests(const vector<string>& tokenise_commands, int client_socket);
    string handle_accept_request(const vector<string>& tokenise_commands, int client_socket);
};

class FileCommandHandler : public BaseCommandHandler {
public:
    FileCommandHandler(TrackerCommandHandler* tracker);
    string handle_upload_file(const vector<string>& tokenise_commands, int client_socket);
    string handle_download_file(const vector<string>& tokenise_commands, int client_socket);
    string handle_stop_share(const vector<string>& tokenise_commands, int client_socket);
};

class TrackerCommandHandler {
private:
    string login;
    map<string, string> users; // userID -> password
    map<string, set<string>> groups; // groupID -> set of userIDs
    map<string, string> group_owner; // groupID -> owner userID
    map<string, set<string>> joining_request; // groupID -> set of requesting userIDs
    map<string, int> one_login; // userID -> login status (0 or 1)
    map<pair<string, int>, unordered_set<int>> chunkownerPorts; // {filename, chunkNum} -> set of ports
    vector<FileInfo> trackerFiles; // Files uploaded to groups
    vector<int> group_list; // List of group IDs
    UserCommandHandler* userHandler;
    GroupCommandHandler* groupHandler;
    RequestCommandHandler* requestHandler;
    FileCommandHandler* fileHandler;

public:
    TrackerCommandHandler();
    ~TrackerCommandHandler();

    // Getters for data access
    map<string, string>& getUsers();
    map<string, set<string>>& getGroups();
    map<string, string>& getGroupOwner();
    map<string, set<string>>& getJoiningRequest();
    map<string, int>& getOneLogin();
    map<pair<string, int>, unordered_set<int>>& getChunkownerPorts();
    vector<FileInfo>& getTrackerFiles();
    string& getLogin();
    vector<int>& getGroupList();

    // Command delegation
    string handleCommand(const vector<string>& tokenise_commands, int client_socket);
};
