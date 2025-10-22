#include "commandhandler.h"
#include "clientutils.h"
#include "FileTransferHandler.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <map>
#include <set>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>

extern std::map<std::string, std::set<std::string>> downloadedFiles;
extern std::map<std::pair<std::string, std::string>, int> stopShared;
extern int check_login;
extern std::string current_loggedin;
extern std::mutex tracker_comm_mutex;
extern std::unordered_set<std::string> st;

// Helper method implementations
string CommandHandler::extract_command_name(const string& command)
{
    string command_name = "";
    for(size_t i = 0; i < command.size(); i++)
    {
        if(command[i] == ' ')
        {
            break;
        }
        command_name += command[i];
    }
    return command_name;
}

bool CommandHandler::check_login_required(const string& command_name, int check_login)
{
    if(check_login == 0)
    {
        if(command_name == "create_group" || command_name == "join_group" || 
           command_name == "list_groups" || command_name == "accept_request" ||
           command_name == "list_requests" || command_name == "leave_group" ||
           command_name == "logout" || command_name == "upload_file" ||
           command_name == "download_file" || command_name == "stop_share" ||
           command_name == "list_files")
        {
            if(command_name == "create_group")
                cout << "First login/Register before creating group" << endl;
            else if(command_name == "join_group")
                cout << "First login/Register before making request to join" << endl;
            else if(command_name == "list_groups")
                cout << "First login/Register before making request to join" << endl;
            else if(command_name == "accept_request")
                cout << "First login/Register before making request to join" << endl;
            else if(command_name == "list_requests")
                cout << "First login/Register before checking list requests" << endl;
            else if(command_name == "leave_group")
                cout << "First login/Register to leave group" << endl;
            else if(command_name == "logout")
                cout << "You are not logged in" << endl;
            else if(command_name == "upload_file" || command_name == "download_file" || 
                    command_name == "stop_share" || command_name == "list_files")
                cout << "You are not logged in" << endl;
            
            return true;
        }
    }
    
    if(command_name == "login" && check_login == 1)
    {
        cout << "you are already logged in" << endl;
        return true;
    }
    
    return false;
}

bool CommandHandler::is_valid_command(const string& command_name, const unordered_set<string>& valid_commands)
{
    if (valid_commands.find(command_name) == valid_commands.end())
    {
        cout << "Invalid command. Please try again." << endl;
        return false;
    }
    return true;
}

void CommandHandler::run_client(const string& ip, int port)
{
    ClientUtils utils;
    FileTransferHandler fileTransferHandler;
    
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        cerr << "Failed to create socket." << endl;
        return;
    }
    vector<string> tokens;
    FILE *file = fopen("tracker_info.txt", "r");
    if (file == nullptr)
    {
        cerr << "Error: Unable to open file." << endl;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        buffer[strcspn(buffer, "\n")] = 0;
        char *token = strtok(buffer, ":");
        while (token != nullptr)
        {
            tokens.push_back(string(token));
            token = strtok(nullptr, ":");
        }
    }
    fclose(file);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(stoi(tokens[1]));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Connection failed." << endl;
        close(client_socket);
        return;
    }

    cout << "Connected to tracker at " << ip << ":" << stoi(tokens[1]) << endl;
    string current_loggedin = "";
    while (true)
    {
        string command;
        cout << "Enter command (or 'quit' to exit): ";
        getline(cin, command);

        // Extract command name
        string compare_command = extract_command_name(command);
        string a = " ";

        // Handle login username extraction
        if(compare_command == "login" && check_login == 0)
        {
            char copied_command[256];
            vector<string> login_tokens;

            strncpy(copied_command, command.c_str(), sizeof(copied_command));
            copied_command[sizeof(copied_command) - 1] = '\0';
            login_tokens = utils.get_tokens_for_command(copied_command, " \t");
            a = login_tokens[1];
        }

        // Check login requirements and handle login/logout status
        if(check_login_required(compare_command, check_login))
        {
            continue;
        }

        // Handle create_group special case
        if(compare_command == "create_group" && check_login == 1)
        {
            cout << current_loggedin << endl;
        }

        // Handle file transfer commands
        if(compare_command == "upload_file" && check_login == 1)
        {
            fileTransferHandler.handle_upload_file(client_socket, command, current_loggedin, ip, port);
            continue;
        }
        else if(compare_command == "download_file" && check_login == 1)
        {
            fileTransferHandler.handle_download_file(client_socket, command, port);
            continue;
        }
        else if(compare_command == "stop_share" && check_login == 1)
        {
            fileTransferHandler.handle_stop_share(command);
            continue;
        }
        else if(compare_command == "create_user")
        {
        }
        
        // Validate command
        if (!is_valid_command(compare_command, st))
        {
            continue;
        }

        // Handle all other commands (forward to tracker)
        if(compare_command != "upload_file" && compare_command != "download_file")
        {
            write(client_socket, command.c_str(), command.size());

            char buffer[20480];
            int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = '\0';
                cout << "Response from tracker: " << buffer << endl;
                if (strcmp(buffer, "Logged in successfully") == 0)
                {
                    current_loggedin += a;
                    check_login = 1;
                }
                if (strcmp(buffer, "Logged out successfully") == 0)
                {
                    check_login = 0;
                    current_loggedin = " ";
                }
            }
            else
            {
                cerr << "Failed to receive response from tracker." << endl;
                break;
            }
            if (command == "quit")
            {
                check_login = 0;
                break;
            }
        }
    }

    close(client_socket);
}