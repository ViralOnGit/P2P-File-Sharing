#include "TrackerCommandHandler.h"
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

extern Utils utils;

BaseCommandHandler::BaseCommandHandler(TrackerCommandHandler* tracker) : tracker(tracker) {}

UserCommandHandler::UserCommandHandler(TrackerCommandHandler* tracker) : BaseCommandHandler(tracker) {}

GroupCommandHandler::GroupCommandHandler(TrackerCommandHandler* tracker) : BaseCommandHandler(tracker) {}

RequestCommandHandler::RequestCommandHandler(TrackerCommandHandler* tracker) : BaseCommandHandler(tracker) {}

FileCommandHandler::FileCommandHandler(TrackerCommandHandler* tracker) : BaseCommandHandler(tracker) {}

TrackerCommandHandler::TrackerCommandHandler() : login(" ") {
    userHandler = new UserCommandHandler(this);
    groupHandler = new GroupCommandHandler(this);
    requestHandler = new RequestCommandHandler(this);
    fileHandler = new FileCommandHandler(this);
}

TrackerCommandHandler::~TrackerCommandHandler() {
    delete userHandler;
    delete groupHandler;
    delete requestHandler;
    delete fileHandler;
}

// Getters implementation
map<string, string>& TrackerCommandHandler::getUsers() { return users; }
map<string, set<string>>& TrackerCommandHandler::getGroups() { return groups; }
map<string, string>& TrackerCommandHandler::getGroupOwner() { return group_owner; }
map<string, set<string>>& TrackerCommandHandler::getJoiningRequest() { return joining_request; }
map<string, int>& TrackerCommandHandler::getOneLogin() { return one_login; }
map<pair<string, int>, unordered_set<int>>& TrackerCommandHandler::getChunkownerPorts() { return chunkownerPorts; }
vector<FileInfo>& TrackerCommandHandler::getTrackerFiles() { return trackerFiles; }
string& TrackerCommandHandler::getLogin() { return login; }
vector<int>& TrackerCommandHandler::getGroupList() { return group_list; }

string TrackerCommandHandler::handleCommand(const vector<string>& tokenise_commands, int client_socket) {
    if (tokenise_commands.empty()) return "";
    string command = tokenise_commands[0];

    // Delegate to appropriate handler
    if (command == "create_user") {
        return userHandler->handle_create_user(tokenise_commands, client_socket);
    } else if (command == "login") {
        return userHandler->handle_login(tokenise_commands, client_socket);
    } else if (command == "logout") {
        return userHandler->handle_logout(tokenise_commands, client_socket);
    } else if (command == "create_group") {
        return groupHandler->handle_create_group(tokenise_commands, client_socket);
    } else if (command == "join_group") {
        return groupHandler->handle_join_group(tokenise_commands, client_socket);
    } else if (command == "list_groups") {
        return groupHandler->handle_list_groups(tokenise_commands, client_socket);
    } else if (command == "leave_group") {
        return groupHandler->handle_leave_group(tokenise_commands, client_socket);
    } else if (command == "list_requests") {
        return requestHandler->handle_list_requests(tokenise_commands, client_socket);
    } else if (command == "accept_request") {
        return requestHandler->handle_accept_request(tokenise_commands, client_socket);
    } else if (command == "upload_file") {
        return fileHandler->handle_upload_file(tokenise_commands, client_socket);
    } else if (command == "download_file") {
        return fileHandler->handle_download_file(tokenise_commands, client_socket);
    } else if (command == "stop_share") {
        return fileHandler->handle_stop_share(tokenise_commands, client_socket);
    }
    return "";
}

string UserCommandHandler::handle_create_user(const vector<string>& tokenise_commands, int client_socket) {
    if(tracker->getUsers().find(tokenise_commands[1])!=tracker->getUsers().end())
    {
        string response = "userID already exists, please create with different user ID";
        write(client_socket, response.c_str(), response.length());
    }
    else
    {
        tracker->getUsers()[tokenise_commands[1]]=tokenise_commands[2];
        tracker->getOneLogin()[tokenise_commands[1]]=0;
        string response = "New User added";
        write(client_socket, response.c_str(), response.length());
    }
    return "";
}

string UserCommandHandler::handle_login(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getUsers().find(tokenise_commands[1])==tracker->getUsers().end())
    {
        string response = "userID Not exists, please register first";
        write(client_socket, response.c_str(), response.length());
    }
    else if(tracker->getOneLogin()[tokenise_commands[1]]==1)
    {
        string response = "You are already logged in";
        write(client_socket, response.c_str(), response.length());
    }
    else if(tokenise_commands[2]==tracker->getUsers()[tokenise_commands[1]])
    {
        tracker->getLogin()=tokenise_commands[1];
        tracker->getOneLogin()[tokenise_commands[1]]=1;
        for (auto& file : tracker->getTrackerFiles()) 
        {
            if (file.clientId == tracker->getLogin())
            {
                file.isLoggedIn = true;
            }
        }
        string response = "Logged in successfully";
        write(client_socket, response.c_str(), response.length());
    }
    else
    {
        string response = "Password is wrong, Try again";
        write(client_socket, response.c_str(), response.length());
    }
    return "";
}

string UserCommandHandler::handle_logout(const vector<string>& tokenise_commands, int client_socket)
{
    (void)tokenise_commands;
    if (tracker->getOneLogin()[tracker->getLogin()] == 0) 
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
    } 
    else 
    {
        for (auto& file : tracker->getTrackerFiles()) //if user logs out then make loggedin=0 so that files he has uploaded doesnt get showed up when he is loggedout
        {
            if (file.clientId == tracker->getLogin()) 
            {
                file.isLoggedIn = false;
            }
        }
        tracker->getOneLogin()[tracker->getLogin()] = 0;  // Mark the user as logged out
        tracker->getLogin() = "";  // Clear the logged-in user for this session
        string response = "Logged out successfully";
        write(client_socket, response.c_str(), response.length());
    }
    return "";
}

string GroupCommandHandler::handle_create_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        tracker->getGroups()[tokenise_commands[1]].insert(tracker->getLogin());
        string response = "created Group successfully";
        tracker->getGroupOwner()[tokenise_commands[1]]=tracker->getLogin();
        tracker->getGroupList().push_back(stoi(tokenise_commands[1]));
        write(client_socket, response.c_str(), response.length());   
    }
    else
    {
        string response = "Group already exists with this id";
        write(client_socket, response.c_str(), response.length());  
    }
    return "";
}

string GroupCommandHandler::handle_join_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
    }
    else if((tracker->getGroupOwner()[tokenise_commands[1]]==tracker->getLogin()))
    {
        string response = "you are already owner of this group";
        write(client_socket, response.c_str(), response.length());  
    }
    else
    {
        tracker->getJoiningRequest()[tokenise_commands[1]].insert(tracker->getLogin());
        string response="joining request sent!";
        write(client_socket, response.c_str(), response.length());  
    }
    return "";
}

string GroupCommandHandler::handle_list_groups(const vector<string>& tokenise_commands, int client_socket)
{
    (void)tokenise_commands;
    if(tracker->getGroupList().size()==0)
    {
        string response = "No groups are there currently";
        write(client_socket, response.c_str(), response.length());  
    }
    else
    {
        string response="Groups are: ";
        for(size_t i=0;i<tracker->getGroupList().size();i++)
        {
            response+=to_string(tracker->getGroupList()[i]);
            response+=" ";
        }
        write(client_socket, response.c_str(), response.length()); 
    }
    return "";
}

string GroupCommandHandler::handle_leave_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "no group with this ID exists";
        write(client_socket, response.c_str(), response.length()); 
        return "";
    }
    auto it=tracker->getGroups().find(tokenise_commands[1]);
    auto &find_userid=it->second;
    if(find_userid.find(tracker->getLogin())==find_userid.end())
    {
        string response = "You are not a member of this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(tracker->getGroupOwner()[tokenise_commands[1]]==tracker->getLogin())
    {
        if(find_userid.size()>1)
        {
            for (auto& new_owner:find_userid) 
            {
                if (new_owner!=tracker->getLogin())
                {
                    tracker->getGroupOwner()[tokenise_commands[1]] = new_owner; 
                    find_userid.erase(tracker->getLogin());
                    string response = "Owner left the group successfully & The new owner is " + new_owner;
                    write(client_socket, response.c_str(), response.length());
                    break;
                }  
            }
        }
        else
        {
            string response = "cant leave as there're no other members than owner";
            write(client_socket, response.c_str(), response.length()); 
            return "";
        }
    }
    else
    {
        find_userid.erase(tracker->getLogin());
        string response = "left group successfully";
        write(client_socket, response.c_str(), response.length());  
    }
    for (auto it = tracker->getTrackerFiles().begin(); it != tracker->getTrackerFiles().end();) //if user leaves group then erase all the files he has uploaded
    {
        if (it->clientId == tracker->getLogin() && it->groupId == tokenise_commands[1]) 
        {
            it = tracker->getTrackerFiles().erase(it);  // Erase and move iterator to the next element
        } 
        else 
        {
            ++it;  // Move to the next element if no erase
        }
    }
    return "";
}

string RequestCommandHandler::handle_list_requests(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
    }
    else if((tracker->getGroupOwner().find(tokenise_commands[1]))->second!=tracker->getLogin())
    {
        string response = "you are not owner of this group";
        write(client_socket, response.c_str(), response.length());  
    }
    else if(tracker->getJoiningRequest().find(tokenise_commands[1])==tracker->getJoiningRequest().end())
    {
        string response = "No joining request for this group exists";
        write(client_socket, response.c_str(), response.length());
    }
    else
    {
        string response="Requests are: ";
        auto it=tracker->getJoiningRequest().find(tokenise_commands[1]);
        for (const string& user : it->second)
        {
            response+=user;
            response+=" ";
        }
        write(client_socket, response.c_str(), response.length()); 
    }
    return "";
}

string RequestCommandHandler::handle_accept_request(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getUsers().find(tokenise_commands[2])==tracker->getUsers().end())
    {
        string response = "userID Not registered, please check user ID";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
        return "";
    }
    if(tracker->getOneLogin()[tracker->getGroupOwner()[tokenise_commands[1]]]==0)
    {
        string response = "You can't accept request of this group, as you are not owner";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto it=tracker->getJoiningRequest().find(tokenise_commands[1]);
    if(it==tracker->getJoiningRequest().end())
    {
        string response = "No joining request are there for this group";
        write(client_socket, response.c_str(), response.length());  
        return "";
    }
    auto &find_userid=it->second;
    if(find_userid.find(tokenise_commands[2])!=find_userid.end())
    {
        find_userid.erase(tokenise_commands[2]);
        tracker->getGroups()[tokenise_commands[1]].insert(tokenise_commands[2]);
        string response = "Joining request ACCEPTED";
        write(client_socket, response.c_str(), response.length());  
    }
    else
    {
        string response = "no joining request found for this user id";
        write(client_socket, response.c_str(), response.length());  
    }
    return "";
}

string FileCommandHandler::handle_upload_file(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getLogin() == "")
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    else if(tracker->getGroups().find(tokenise_commands[2])==tracker->getGroups().end())
    {
        string response = "group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto &find_userid = tracker->getGroups()[tokenise_commands[2]];
    if(find_userid.find(tracker->getLogin())==find_userid.end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length());
        return "";
    } 
    string filename = tokenise_commands[1];
    string groupid = tokenise_commands[2];
    string port = tokenise_commands[8];
    vector<string> chunkSHA1s;
    for(size_t i=9; i < tokenise_commands.size()-1; ++i)
        chunkSHA1s.push_back(tokenise_commands[i]);
    bool already_uploaded = false;
    for(const auto& info : tracker->getTrackerFiles()) {
        if(info.fileName == filename && info.groupId == groupid && info.clientId == tracker->getLogin()) {
            already_uploaded = true;
            break;
        }
    }
    if(already_uploaded) {
        string response = "You have already uploaded this file in this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    FileInfo file = {filename, groupid, tracker->getLogin(), port, chunkSHA1s, true, false};
    tracker->getTrackerFiles().push_back(file);
    for (size_t i = 0; i < chunkSHA1s.size(); ++i) {
        tracker->getChunkownerPorts()[{filename, i}].insert(stoi(port)); 
    }
    string response = "File uploaded successfully";
    write(client_socket, response.c_str(), response.size());
    return "";
}
/*
    checks edge conditions and then insert the upload request data to trackerfiles for metadata
    lookups in other commands(i.e storing chunkhashes, file is available to which group) etc, atleast one user is 
    there who has file and is loggedin and stop_share = false for that, (2 level checkup)
    at client side also we are checking after rreceiving ports that is the port is loggedin and
    stop-share = false or not and tracker side also we are checking before sending, so we kept that
    both in trackerfiles vector.
    and then we insert the filename, chunknumber -> port in chunkownerports too to indicate which chunk
    has which ports that are sharing it. and so necessary info is stored like that.
*/

string FileCommandHandler::handle_download_file(const vector<string>& tokenise_commands, int client_socket)
{
    if(tracker->getLogin() == "")
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    else if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto &find_userid = tracker->getGroups()[tokenise_commands[1]];
    if(find_userid.find(tracker->getLogin())==find_userid.end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    string groupid = tokenise_commands[1];
    string filename = tokenise_commands[2];
    string filesize, file_sha1;
    vector<string> chunk_sha1s;
    bool file_available = false;
    
    // Use trackerFiles to check availability and get chunk hashes
    for(const auto& fileInfo : tracker->getTrackerFiles()) {
        if(fileInfo.fileName == filename && fileInfo.groupId == groupid && fileInfo.isLoggedIn && !fileInfo.stop_share) {
            file_available = true;
            if(chunk_sha1s.empty()) {
                chunk_sha1s = fileInfo.chunkSHA1s;
            }
        }
    }
    
    if(!file_available) {
        string response = "Sorry, Currently this file is not available in this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    string message;
    for(const auto& chunk_sha : chunk_sha1s) {
        message += chunk_sha + " ";
    }
    message += ";";
    for (size_t i = 0; i < chunk_sha1s.size(); ++i) {
        message += to_string(i) + " ";
        for(auto &ports : tracker->getChunkownerPorts()[{filename,i}]) {
            message += to_string(ports) + " ";
        }
        message += ";";
    }
    write(client_socket, message.c_str(), message.length());
    while (true) {
        char have_chunk_buffer[1024];
        int have_chunk_bytes = read(client_socket, have_chunk_buffer, sizeof(have_chunk_buffer) - 1);
        cout << "Received have_chunk message: " << have_chunk_buffer << endl;
        vector<string> have_tokens;
        if (have_chunk_bytes > 0) {
            have_chunk_buffer[have_chunk_bytes] = '\0';
            have_tokens = utils.get_tokens_for_commands(have_chunk_buffer, ";");
            for(size_t i = 0; i < have_tokens.size(); ++i) {
                cout << "have_tokens[" << i << "]: " << have_tokens[i] << endl;
            }
            if(have_tokens[0] == "have_chunk") {
                string have_filename = have_tokens[1];
                int have_chunkNum = stoi(have_tokens[2]);
                int have_port = stoi(have_tokens[3]);
                cout << have_filename << " " << have_chunkNum << " " << have_port << endl;
                tracker->getChunkownerPorts()[{have_filename, have_chunkNum}].insert(have_port);
                
                string ack = "Chunk registered";
                write(client_socket, ack.c_str(), ack.length());
            } else if(have_tokens[0] == "DONE") {
                cout << "Client finished downloading chunks." << endl;
                break;
            } else if(have_tokens[0] == "NOT_DONE") {
                cout << "Client did not finish downloading chunks." << endl;
                break;
            }
        }
    }
    return "";
}
/*
    checks edge conditions and then uses trackerfiles to check if atleast one user is 
    there who has file and is loggedin and stop_share = false for that, (2 level checkup)
    at client side also we are checking after rreceiving ports that is the port is loggedin and
    stop-share = false or not and tracker side also we are checking before sending, so we kept that
    both in trackerfiles vector.
    and then we lookup the filename, chunknumber -> port in chunkownerports to indicate which chunk
    has which ports that are sharing it. and so necessary info is sent back to client like that.
    and also we read back from client which chunks he has successfully downloaded and update
    chunkownerports accordingly to add his port as well for those chunks. and at the end
    if client is done downloading all chunks we receive DONE message else NOT_DONE message.
*/

string FileCommandHandler::handle_stop_share(const vector<string>& tokenise_commands, int client_socket)
{
    if(tokenise_commands.size()<3)
    {
        string response = "Please provide file name to stop sharing";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(tracker->getGroups().find(tokenise_commands[1])==tracker->getGroups().end())
    {
        string response = "Group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(tracker->getOneLogin()[tracker->getLogin()] == 0)
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto it=tracker->getGroups()[tokenise_commands[1]].find(tracker->getLogin());
    if(it==tracker->getGroups()[tokenise_commands[1]].end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length()); 
        return "";
    } 
    int flag = 0;
    string message="";
    for (auto& fileInfo : tracker->getTrackerFiles()) 
    {
        if (fileInfo.groupId == tokenise_commands[1] && fileInfo.fileName == tokenise_commands[2]) 
        {
            if(fileInfo.clientId == tracker->getLogin() && fileInfo.stop_share == false)
            {
                flag = 1;
                fileInfo.stop_share = true;
                for(size_t i=0;i<fileInfo.chunkSHA1s.size();i++){
                    tracker->getChunkownerPorts()[{fileInfo.fileName, i}].erase(stoi(fileInfo.port));
                }
                message+="File sharing stopped for " + fileInfo.fileName + " in group " + fileInfo.groupId + "By User: " + tracker->getLogin() + " ";
                write(client_socket, message.c_str(), message.length()); 
                break;
            }
            else if(fileInfo.clientId == tracker->getLogin() && fileInfo.stop_share == true)
            {
                message+="You are not sharing this file.";
                write(client_socket, message.c_str(), message.length());
            }
        }
    }
    if(flag == 0)
    {
        message+="File not found in this group.";
        write(client_socket, message.c_str(), message.length());
    }
    return "";
}