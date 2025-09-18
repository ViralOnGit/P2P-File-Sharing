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

        // Tokenize the command
        vector<string> tokenise_commands = utils.get_tokens_for_commands(buffer, " \t");

        // Handle the quit command by converting it to logout
        if (strcmp(buffer, "quit") == 0) 
        {
            tokenise_commands.clear();
            tokenise_commands.push_back("logout");
            commandHandler.handleCommand(tokenise_commands, client_socket);
            close(client_socket);
            break; 
        }

        // Delegate all commands to TrackerCommandHandler
        string response = commandHandler.handleCommand(tokenise_commands, client_socket);
        if (!response.empty()) {
            write(client_socket, response.c_str(), response.length());
        }
    }
    close(client_socket);
    cout << "Client disconnected." << endl;
}

int main(int argc, char *argv[]) 
{
    if (argc != 3)
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