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
#include "utils.h"
#include "TrackerCommandHandler.h"
using namespace std;
Utils utils;

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
    tokens=utils.get_tokens(argv[1],":");//tokenising to get tracker ip and port
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

        // Create thread to handle client
        tids.push_back(thread(handle_ClientFunctions, client_socket));
    }
    // Close the tracker socket
    close(server_socket);
    return 0;
}
