#include "TrackerCommandHandler.h"
#include "utils.h"
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <iostream>

using namespace std;

extern Utils utils;

TrackerCommandHandler::TrackerCommandHandler() : login(" ") {}

// Getters for external access to data structures
map<string, string>& TrackerCommandHandler::getUsers() { return users; }
map<string, set<string>>& TrackerCommandHandler::getGroups() { return groups; }
map<string, string>& TrackerCommandHandler::getGroupOwner() { return group_owner; }
map<string, set<string>>& TrackerCommandHandler::getJoiningRequest() { return joining_request; }
map<string, int>& TrackerCommandHandler::getOneLogin() { return one_login; }
map<pair<string, int>, unordered_set<int>>& TrackerCommandHandler::getChunkownerPorts() { return chunkownerPorts; }
vector<FileInfo>& TrackerCommandHandler::getTrackerFiles() { return trackerFiles; }
vector<FileInfo>& TrackerCommandHandler::getTotalClients() { return totalclients; }
string& TrackerCommandHandler::getLogin() { return login; }
vector<int>& TrackerCommandHandler::getGroupList() { return group_list; }

string TrackerCommandHandler::handle_create_user(const vector<string>& tokenise_commands, int client_socket)
{
    if(users.find(tokenise_commands[1])!=users.end())
    {
        string response = "userID already exists, please create with different user ID";
        write(client_socket, response.c_str(), response.length());
    }
    else
    {
        users[tokenise_commands[1]]=tokenise_commands[2];
        one_login[tokenise_commands[1]]=0;
        string response = "New User added";
        write(client_socket, response.c_str(), response.length());
    }
    return "";
}

string TrackerCommandHandler::handle_login(const vector<string>& tokenise_commands, int client_socket)
{
    if(users.find(tokenise_commands[1])==users.end())
    {
        string response = "userID Not exists, please register first";
        write(client_socket, response.c_str(), response.length());
    }
    else if(one_login[tokenise_commands[1]]==1)
    {
        string response = "You are already logged in";
        write(client_socket, response.c_str(), response.length());
    }
    else if(tokenise_commands[2]==users[tokenise_commands[1]])
    {
        

        login=tokenise_commands[1];
        one_login[tokenise_commands[1]]=1;
        for (auto& file : trackerFiles) 
        {
            if (file.clientId == login)
            {
                file.isLoggedIn = true;//if logged in again then start showing them file that user has uploaded previously before logout
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

string TrackerCommandHandler::handle_create_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        groups[tokenise_commands[1]].insert(login);
        string response = "created Group successfully";
        group_owner[tokenise_commands[1]]=login;
        this->group_list.push_back(stoi(tokenise_commands[1]));
        write(client_socket, response.c_str(), response.length());   
    }
    else
    {
        string response = "Group already exists with this id";
        write(client_socket, response.c_str(), response.length());  

    }
    return "";
}

string TrackerCommandHandler::handle_join_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
    }
    else if((group_owner[tokenise_commands[1]]==login))//checking if owner is sending the request
    {
        string response = "you are already owner of this group";
        write(client_socket, response.c_str(), response.length());  

    }
    //check if already there in group
    //check if already request is placed.
    else
    {
        joining_request[tokenise_commands[1]].insert(login);
        string response="joining request sent!";
        write(client_socket, response.c_str(), response.length());  

    }
    return "";
}

string TrackerCommandHandler::handle_list_requests(const vector<string>& tokenise_commands, int client_socket)
{
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
    }
    else if((group_owner.find(tokenise_commands[1]))->second!=login)//checking if owner is sending the request
    {
        string response = "you are not owner of this group";
        write(client_socket, response.c_str(), response.length());  

    }
    else if(joining_request.find(tokenise_commands[1])==joining_request.end())
    {
        string response = "No joining request for this group exists";
        write(client_socket, response.c_str(), response.length());
    }
    else
    {
        string response="Requests are: ";
        auto it=joining_request.find(tokenise_commands[1]);
            for (const string& user : it->second)
            {
                response+=user;
                response+=" ";
            }
            write(client_socket, response.c_str(), response.length()); 
    }
    return "";
}

string TrackerCommandHandler::handle_list_groups(const vector<string>& tokenise_commands, int client_socket)
{
    if(this->group_list.size()==0)
    {
        string response = "No groups are there currently";
        write(client_socket, response.c_str(), response.length());  
    }
    else
    {
        string response="Groups are: ";
        for(int i=0;i<this->group_list.size();i++)
        {
            response+=to_string(this->group_list[i]);
            response+=" ";
            
        }
        write(client_socket, response.c_str(), response.length()); 
    }
    return "";
}

string TrackerCommandHandler::handle_accept_request(const vector<string>& tokenise_commands, int client_socket)
{
    if(users.find(tokenise_commands[2])==users.end())
    {
        string response = "userID Not registered, please check user ID";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "Group doesn't exists with this id";
        write(client_socket, response.c_str(), response.length());  
        return "";
    }
    if(one_login[group_owner[tokenise_commands[1]]]==0)
    {
        string response = "You can't accept request of this group, as you are not owner";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto it=joining_request.find(tokenise_commands[1]);
    if(it==joining_request.end())
    {
        string response = "No joining request are there for this group";
        write(client_socket, response.c_str(), response.length());  
        return "";
    }
    auto &find_userid=it->second;
    if(find_userid.find(tokenise_commands[2])!=find_userid.end())
    {
        find_userid.erase(tokenise_commands[2]);
        groups[tokenise_commands[1]].insert(tokenise_commands[2]);
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

string TrackerCommandHandler::handle_leave_group(const vector<string>& tokenise_commands, int client_socket)
{
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "no group with this ID exists";
        write(client_socket, response.c_str(), response.length()); 
        return "";
    }
    auto it=groups.find(tokenise_commands[1]);
    auto &find_userid=it->second;
    if(find_userid.find(login)==find_userid.end())
    {
        string response = "You are not a member of this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(group_owner[tokenise_commands[1]]==login)
    {
        if(find_userid.size()>1)
        {
            for (auto& new_owner:find_userid) 
            {
                if (new_owner!=login)
                {
                    group_owner[tokenise_commands[1]] = new_owner; 
                    find_userid.erase(login);
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
        find_userid.erase(login);
        string response = "left group successfully";
        write(client_socket, response.c_str(), response.length());  
    }

    for (auto it = trackerFiles.begin(); it != trackerFiles.end();) //if user leaves group then erase all the files he has uploaded
    {
        if (it->clientId == login && it->groupId == tokenise_commands[1]) 
        {
            it = trackerFiles.erase(it);  // Erase and move iterator to the next element
        } 
        else 
        {
            ++it;  // Move to the next element if no erase
        }
    }
    return "";
}

string TrackerCommandHandler::handle_logout(const vector<string>& tokenise_commands, int client_socket)
{
    if (one_login[login] == 0) 
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
    } 
    else 
    {
        for (auto& file : trackerFiles) //if user logs out then make loggedin=0 so that files he has uploaded doesnt get showed up when he is loggedout
        {
            if (file.clientId == login) 
            {
                file.isLoggedIn = false;
            }
        }
        for (auto& file : totalclients) //if user logs out then make loggedin=0 so that files he has uploaded doesnt get showed up when he is loggedout
        {
            if (file.clientId == login) 
            {
                file.isLoggedIn = false;
            }
        }
        one_login[login] = 0;  // Mark the user as logged out
        login = "";  // Clear the logged-in user for this session
        string response = "Logged out successfully";
        write(client_socket, response.c_str(), response.length());
    }
    return "";
}

string TrackerCommandHandler::handle_upload_file(const vector<string>& tokenise_commands, int client_socket)
{
    if(login == "")
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    else if(groups.find(tokenise_commands[2])==groups.end())
    {
        string response = "group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto &find_userid = groups[tokenise_commands[2]];
    if(find_userid.find(login)==find_userid.end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length());
        return "";
    } 

    string filename = tokenise_commands[1];
    string groupid = tokenise_commands[2];
    string port = tokenise_commands[8];
    vector<string> chunkSHA1s;
    for(int i=9; i < tokenise_commands.size()-1; ++i)
        chunkSHA1s.push_back(tokenise_commands[i]);

    // Check if this client already uploaded this file in this group
    bool already_uploaded = false;
    for(const auto& info : trackerFiles) {
        if(info.fileName == filename && info.groupId == groupid && info.clientId == login) {
            already_uploaded = true;
            break;
        }
    }
    if(already_uploaded) {
        string response = "You have already uploaded this file in this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }

    FileInfo file = {filename, groupid, login, port, chunkSHA1s, true, false};

    trackerFiles.push_back(file);
    totalclients.push_back(file);
    
    for (size_t i = 0; i < chunkSHA1s.size(); ++i) // Update the chunkownerPorts map with chunks of this file are shared by this port
    {
        chunkownerPorts[{filename, i}].insert(stoi(port)); 
    }
    string response = "File uploaded successfully";
    write(client_socket, response.c_str(), response.size());
    return "";
}

string TrackerCommandHandler::handle_download_file(const vector<string>& tokenise_commands, int client_socket)
{
    if(login == "")
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    else if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto &find_userid = groups[tokenise_commands[1]];
    if(find_userid.find(login)==find_userid.end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }

    string groupid = tokenise_commands[1];
    string filename = tokenise_commands[2];

    // Find all clients in this group who have the file and are logged in and sharing
    vector<FileInfo> file_holders;
    string filesize, file_sha1;
    vector<string> chunk_sha1s;
    
    for(const auto& fileInfo : totalclients) 
    {
        if(fileInfo.fileName == filename && fileInfo.groupId == groupid && fileInfo.isLoggedIn && !fileInfo.stop_share) 
        {
            file_holders.push_back(fileInfo);
            if(chunk_sha1s.size() < fileInfo.chunkSHA1s.size()) 
            {
                chunk_sha1s = fileInfo.chunkSHA1s;
            }
        }
    }

    if(file_holders.empty()) 
    {
        string response = "Sorry, Currently this file is not available in this group";
        write(client_socket, response.c_str(), response.length());
        return "";
    }

    // Send chunk SHA1s to client 
    string message;
    for(const auto& chunk_sha : chunk_sha1s) 
    {
        message += chunk_sha + " ";
    }
    message += ";";
    for (size_t i = 0; i < chunk_sha1s.size(); ++i) // Update the chunkownerPorts map with chunks of this file are shared by this port
    {
        message += to_string(i) + " ";
        for(auto &ports : chunkownerPorts[{filename,i}])
        {
            message += to_string(ports) + " ";
        }
        message += ";";
    }

    write(client_socket, message.c_str(), message.length()); //whole message sent with chunk SHA1s and chunk no and its ports

    // --- Main per-chunk loop ---
    while (true) 
    {

                // Wait for "have_chunk" message from client (after successful download)
                char have_chunk_buffer[1024];
                int have_chunk_bytes = read(client_socket, have_chunk_buffer, sizeof(have_chunk_buffer) - 1);
                cout << "Received have_chunk message: " << have_chunk_buffer << endl;
                vector<string> have_tokens;
                if (have_chunk_bytes > 0) 
                {
                    have_chunk_buffer[have_chunk_bytes] = '\0';
                    have_tokens = utils.get_tokens_for_commands(have_chunk_buffer, ";");
                    for(int i = 0; i < have_tokens.size(); ++i) 
                    {
                        cout << "have_tokens[" << i << "]: " << have_tokens[i] << endl;
                    }
                    if(have_tokens[0] == "have_chunk") 
                    {
                        // Format: have_chunk <groupid> <filename> <chunkNum> <port>
                        string have_filename = have_tokens[1];
                        int have_chunkNum = stoi(have_tokens[2]);
                        int have_port = stoi(have_tokens[3]);
                        cout << have_filename << " " << have_chunkNum << " " << have_port << endl;
                        chunkownerPorts[{have_filename, have_chunkNum}].insert(have_port);
                        // Try to find an existing FileInfo for this file and port
                        bool found = false;
                        for (auto& info : totalclients) 
                        {
                            if (info.fileName == have_filename && info.port == to_string(have_port)) 
                            {
                                // Set the SHA1 for this chunk (if you have it)
                                info.chunkSHA1s[have_chunkNum] = chunk_sha1s[have_chunkNum];
                                found = true;
                                break;
                            }
                        }

                if (!found) 
                {
                    FileInfo newInfo;
                    newInfo.fileName = have_filename;
                    newInfo.groupId = groupid;        // get from message/context
                    newInfo.clientId = login;      // get from session/context
                    newInfo.port = to_string(have_port);
                    newInfo.isLoggedIn = true;
                    newInfo.stop_share = false;
                    // Initialize chunkSHA1s vector
                    newInfo.chunkSHA1s.resize(chunk_sha1s.size(), ""); // Fill with empty strings
                    newInfo.chunkSHA1s[have_chunkNum] = chunk_sha1s[have_chunkNum]; // Set the SHA1 for this chunk
                    totalclients.push_back(newInfo);
                }   
                
                string ack = "Chunk registered";
                write(client_socket, ack.c_str(), ack.length());
            }
            else if(have_tokens[0] == "DONE")
            {
                cout << "Client finished downloading chunks." << endl;
                break; // Exit the per-chunk loop if client is done
            }
            else if(have_tokens[0] == "NOT_DONE")
            {
                cout << "Client did not finish downloading chunks." << endl;
                break; // Exit the per-chunk loop if client is done
            }

        }
        
        // Loop continues for next chunk
    }
    return "";
}

string TrackerCommandHandler::handle_stop_share(const vector<string>& tokenise_commands, int client_socket)
{
    if(tokenise_commands.size()<3)
    {
        string response = "Please provide file name to stop sharing";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(groups.find(tokenise_commands[1])==groups.end())
    {
        string response = "Group with this ID doesn't exist";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    if(one_login[login] == 0)
    {
        string response = "You are not logged in";
        write(client_socket, response.c_str(), response.length());
        return "";
    }
    auto it=groups[tokenise_commands[1]].find(login);
    if(it==groups[tokenise_commands[1]].end())
    {
        string response = "You are not even part of the group";
        write(client_socket, response.c_str(), response.length()); 
        return "";
    } 
    int flag = 0;
    string message="";

    for (auto& fileInfo : trackerFiles) 
    {
        if (fileInfo.groupId == tokenise_commands[1] && fileInfo.fileName == tokenise_commands[2]) 
        {
            if(fileInfo.clientId == login && fileInfo.stop_share == false)
            {
                flag = 1;
                fileInfo.stop_share = true;
                for(int i=0;i<fileInfo.chunkSHA1s.size();i++){
                    chunkownerPorts[{fileInfo.fileName, i}].erase(stoi(fileInfo.port));
                }
                
                message+="File sharing stopped for " + fileInfo.fileName + " in group " + fileInfo.groupId + "By User: " + login + " ";
                write(client_socket, message.c_str(), message.length()); 
                break;
            }
            else if(fileInfo.clientId == login && fileInfo.stop_share == true)
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
