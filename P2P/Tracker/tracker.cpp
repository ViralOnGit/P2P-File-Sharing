#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <thread>
using namespace std;

//class of fileInfo

int create_trackerSocket()
{
    int createSocket=socket(AF_INET, SOCK_STREAM, 0);
    if (createSocket < 0) 
    {
        cout << "Socket for tracker is not created" << endl;
        return -1;
    }
    return createSocket;

}
vector<string> get_tokens_for_commands(char buffer[1024],const char *delimiter)
{
    vector<string> tokens;
   
        char *token = strtok(buffer, delimiter); 
        while (token != nullptr) 
        {
            tokens.push_back(string(token)); 
            token = strtok(nullptr, delimiter);
        }
    
 
    return tokens;
}
vector<string> get_tokens(const char *filename,const char *delimiter)
{
    vector<string> tokens;
    FILE *file = fopen(filename, "r");
    if (file == nullptr) 
    {
        cerr << "Error: Unable to open file." << endl;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) 
    {
        // Strip the newline character if it exists
        buffer[strcspn(buffer, "\n")] = 0;
        char *token = strtok(buffer, delimiter); 
        while (token != nullptr) 
        {
            tokens.push_back(string(token)); 
            token = strtok(nullptr, delimiter);
        }
    }
    fclose(file);
    return tokens;

}
map<string,string> users;//key: user id, value: user password
map<string,set<string>> groups;//key: group id
map<string,string> group_owner;//key: group id, value: owner id of that groups
map<string,set<string>> joining_request;
map<string,int> one_login;
map<pair<string, int>, unordered_set<int>> chunkownerPorts;

thread_local string login=" ";
vector<int> group_list;
struct FileInfo //YES
{
    std::string fileName;                // Name of the file
    std::string groupId;                 // sGroup ID to which the file belongs
    std::string clientId;                // ID of the client who has the file
    // std::string filepath;                // Path to the file on the client
    // std::string filesize;                // Size of the file (as string)
    // std::string SHA1;                    // SHA1 of the entire file
    std::string port;                    // Port where the client is serving
    // std::string ip;                      // IP address of the client (optional, but recommended)
    std::vector<std::string> chunkSHA1s; // SHA1 hashes for each chunk (for integrity verification)
    bool isLoggedIn;                     // Whether the client is currently logged in
    bool stop_share;                     // Whether client is still sharing the file
};
// filename, groupid, login, filepath, filesize, sha1, port, chunkSHA1s, true, true, ip

vector<FileInfo> trackerFiles;//created to store fileinfo for list_files
vector<FileInfo> totalclients;
void handle_ClientFunctions(int client_socket) 
{
    while (true) 
    {
        char buffer[20480];
        int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_received <= 0) 
        {
            cerr << "Failed to receive data from client or client disconnected." << endl;
            break; 
        }

        buffer[bytes_received] = '\0'; 
        cout << "Received from client: " << buffer << endl;
        if (strcmp(buffer, "quit") == 0) 
        {
            // cout << login << "hii" << endl;
            // if(login != " ")
            // {
            string response = "User is logged out";
            write(client_socket, response.c_str(), response.length());
            if(login != " ")
            {
                one_login[login] = 0;
                login=" ";
            }
            close(client_socket);

            
            break; 
        }
        vector<string> tokenise_commands;
        tokenise_commands=get_tokens_for_commands(buffer," \t");
        if(tokenise_commands[0]=="create_user")
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

        }
        else if(tokenise_commands[0]=="login")
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
        }
        else if(tokenise_commands[0]=="create_group")
        {
            if(groups.find(tokenise_commands[1])==groups.end())
            {
                groups[tokenise_commands[1]].insert(login);
                string response = "created Group successfully";
                group_owner[tokenise_commands[1]]=login;
                group_list.push_back(stoi(tokenise_commands[1]));
                write(client_socket, response.c_str(), response.length());   
            }
            else
            {
                string response = "Group already exists with this id";
                write(client_socket, response.c_str(), response.length());  

            }
        }
        else if(tokenise_commands[0]=="join_group")
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
        }
        else if(tokenise_commands[0]=="list_requests")
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
        }
        else if(tokenise_commands[0]=="list_groups")
        {
            if(group_list.size()==0)
            {
                string response = "No groups are there currently";
                write(client_socket, response.c_str(), response.length());  
            }
            else
            {
                string response="Groups are: ";
                for(int i=0;i<group_list.size();i++)
                {
                    response+=to_string(group_list[i]);
                    response+=" ";
                    
                }
                write(client_socket, response.c_str(), response.length()); 
            }
        }
        else if(tokenise_commands[0]=="accept_request")
        {
            if(users.find(tokenise_commands[2])==users.end())
            {
                string response = "userID Not registered, please check user ID";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            if(groups.find(tokenise_commands[1])==groups.end())
            {
                string response = "Group doesn't exists with this id";
                write(client_socket, response.c_str(), response.length());  
                continue;
            }
            if(one_login[group_owner[tokenise_commands[1]]]==0)
            {
                string response = "You can't accept request of this group, as you are not owner";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            auto it=joining_request.find(tokenise_commands[1]);
            if(it==joining_request.end())
            {
                string response = "No joining request are there for this group";
                write(client_socket, response.c_str(), response.length());  
                continue;
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

        }
        else if(tokenise_commands[0]=="leave_group")
        {
            if(groups.find(tokenise_commands[1])==groups.end())
            {
                string response = "no group with this ID exists";
                write(client_socket, response.c_str(), response.length()); 
                continue;
            }
            auto it=groups.find(tokenise_commands[1]);
            auto &find_userid=it->second;
            if(find_userid.find(login)==find_userid.end())
            {
                string response = "You are not a member of this group";
                write(client_socket, response.c_str(), response.length());
                continue;
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
                    continue;
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
            // auto it=groups.find(tokenise_commands[1]);
            // auto &find_userid=it->second;
            
               
            
        }
        else if (tokenise_commands[0] == "logout") 
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
        }
        else if(tokenise_commands[0] == "upload_file")
        {
            if(login == "")
            {
                string response = "You are not logged in";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            else if(groups.find(tokenise_commands[2])==groups.end())
            {
                string response = "group with this ID doesn't exist";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            auto &find_userid = groups[tokenise_commands[2]];
            if(find_userid.find(login)==find_userid.end())
            {
                string response = "You are not even part of the group";
                write(client_socket, response.c_str(), response.length());
                continue;
            } 

            string filename = tokenise_commands[1];
            string groupid = tokenise_commands[2];
            // string filesize = tokenise_commands[3];//not needed
            // string sha1 = tokenise_commands[4];//not needed
            // string filepath = tokenise_commands[6];//not needed
            // string ip = tokenise_commands[7];//not needed
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
                continue;
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
        }
        else if(tokenise_commands[0]=="download_file")
        {
            if(login == "")
            {
                string response = "You are not logged in";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            else if(groups.find(tokenise_commands[1])==groups.end())
            {
                string response = "group with this ID doesn't exist";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            auto &find_userid = groups[tokenise_commands[1]];
            if(find_userid.find(login)==find_userid.end())
            {
                string response = "You are not even part of the group";
                write(client_socket, response.c_str(), response.length());
                continue;
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
                continue;
            }

            // Send chunk SHA1s to client (semicolon delimited)
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

            // cout << message << endl;
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
                    have_tokens = get_tokens_for_commands(have_chunk_buffer, ";");
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
        }


    
        else if(tokenise_commands[0]=="stop_share")
        {
    // Iterate over the trackerFiles vector to find the matching file
            if(tokenise_commands.size()<3)
            {
                string response = "Please provide file name to stop sharing";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            if(groups.find(tokenise_commands[1])==groups.end())
            {
                string response = "Group with this ID doesn't exist";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            if(one_login[login] == 0)
            {
                string response = "You are not logged in";
                write(client_socket, response.c_str(), response.length());
                continue;
            }
            auto it=groups[tokenise_commands[1]].find(login);
            // auto &find_userid=it->second;
            if(it==groups[tokenise_commands[1]].end())
            {
                string response = "You are not even part of the group";
                write(client_socket, response.c_str(), response.length()); 
                continue;
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
            // if(flag == 1)
            // {
            //     message+="You don't even share this file.";
            //     write(client_socket, message.c_str(), message.length());
            // }
            if(flag == 0)
            {
                message+="File not found in this group.";
                write(client_socket, message.c_str(), message.length());
            }
        
        }
        else
        {
            string response = "Invalid command. Please try again.";
            write(client_socket, response.c_str(), response.length());
            continue;
        }
        
    }
    close(client_socket);
    cout << "Client disconnected." << endl;
}
int main(int argc, char *argv[]) 
{
    int opt=1;
    if(argc != 3)
    {
        cerr << "There must be exactly 3 arguments for tracker" << endl;
        return 1;
    }

    // socket creation
    int server_socket = create_trackerSocket();
    if (server_socket < 0) {
        cout << "Socket for tracker is not created" << endl;
        return 1;
    }
    if (setsockopt(server_socket, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    vector<string> tokens;
    tokens=get_tokens(argv[1],":");//tokenising to get tracker ip and port
    string tracker_ip_address =tokens[0];
    int tracker_port = stoi(tokens[1]);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(tracker_ip_address.c_str());  // Convert IP to binary
    server_addr.sin_port = htons(tracker_port);  // Convert port to network byte order
    inet_pton(AF_INET, tracker_ip_address.c_str(), &(server_addr.sin_addr));


    // bind the socket to the IP and port
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed." << endl;
        close(server_socket);
        return 1;
    }
    // listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        cerr << "Listen failed." << endl;
        close(server_socket);
        return 1;
    }

    cout << "Tracker is running on " << tracker_ip_address << ":" << tracker_port << endl;

    // Main loop to accept client connections
    vector<thread> tids;
    int client_socket;
    while (true) {
        // Accept the incoming client connection
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "Accept failed." << endl;
            continue;
        }

        cout<<"Client and Tracker Connected!"<<endl;

        // Keep reading data from client until it sends "quit"
        // handle_ClientFunctions(client_socket);
        tids.push_back(thread(handle_ClientFunctions,client_socket));
    }
    // Close the tracker socket
    close(server_socket);
    return 0;
}
