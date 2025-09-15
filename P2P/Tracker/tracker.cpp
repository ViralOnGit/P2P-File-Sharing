#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <unistd.h>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <thread>
#include "utils.h"
#include "TrackerCommandHandler.h"
#include "TrackerSocket.h"
using namespace std;
Utils utils;

// Global TrackerCommandHandler instance
TrackerCommandHandler commandHandler;

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
            string response = "User is logged out";
            (void)write(client_socket, response.c_str(), response.length());
            string& login = commandHandler.getLogin();
            if(login != " ")
            {
                commandHandler.getOneLogin()[login] = 0;
                login = " ";
            }
            close(client_socket);
            break; 
        }
        vector<string> tokenise_commands;
        tokenise_commands=utils.get_tokens_for_commands(buffer," \t");
        
        if(tokenise_commands[0]=="create_user")
        {
            commandHandler.handle_create_user(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="login")
        {
            commandHandler.handle_login(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="create_group")
        {
            commandHandler.handle_create_group(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="join_group")
        {
            commandHandler.handle_join_group(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="list_requests")
        {
            commandHandler.handle_list_requests(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="list_groups")
        {
            commandHandler.handle_list_groups(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="accept_request")
        {
            commandHandler.handle_accept_request(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="leave_group")
        {
            commandHandler.handle_leave_group(tokenise_commands, client_socket);
        }
        else if (tokenise_commands[0] == "logout") 
        {
            commandHandler.handle_logout(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0] == "upload_file")
        {
            commandHandler.handle_upload_file(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="download_file")
        {
            commandHandler.handle_download_file(tokenise_commands, client_socket);
        }
        else if(tokenise_commands[0]=="stop_share")
        {
            commandHandler.handle_stop_share(tokenise_commands, client_socket);
        }
        else
        {
            string response = "Invalid command. Please try again.";
            (void)write(client_socket, response.c_str(), response.length());
            continue;
        }
        
    }
    close(client_socket);
    cout << "Client disconnected." << endl;
}

int main(int argc, char *argv[]) 
{
    if(argc != 3)
    {
        cerr << "There must be exactly 3 arguments for tracker" << endl;
        return 1;
    }

    // Parse tracker IP and port
    vector<string> tokens;
    tokens = utils.get_tokens(argv[1], ":");
    string tracker_ip_address = tokens[0];
    int tracker_port = stoi(tokens[1]);

    TrackerSocket server(tracker_ip_address, tracker_port);

    // Initialize the server
    if (!server.init()) {
        cerr << "Failed to initialize server" << endl;
        return 1;
    }

    // Start the server (this will run indefinitely)
    server.run();

    return 0;
}
