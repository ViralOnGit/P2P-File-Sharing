#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <sys/stat.h>
#include <map>
#include <set>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_set>
#include "clientutils.h"
#include "clientsocket.h"
#include "commandhandler.h"
#include "FileTransferHandler.h"

using namespace std;
std::mutex tracker_comm_mutex;
map<string, set<string>> downloadedFiles;
map<pair<string, string>, int> stopShared;
int check_login = 0;
string current_loggedin = " ";
unordered_set<string> st = {"quit", "login", "create_group", "join_group", "list_groups", "accept_request", "list_requests", "leave_group", "logout", "upload_file", "stop_share", "download_file", "create_user"};

int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        cerr << "Arguments must be 3 in initialising the client" << endl;
        return 1;
    }

    char* ip_port = argv[1];
    char* tracker_info_file = argv[2];

    char* ip = strtok(ip_port, ":");
    char* port_str = strtok(nullptr, ":");

    if (ip == nullptr || port_str == nullptr) {
        cerr << "Error: Invalid format for <IP>:<PORT>. Expected format is <IP>:<PORT>." << endl;
        return 1;
    }

    int port = atoi(port_str);

    cout << "IP Address: " << ip << endl;
    cout << "Port: " << port << endl;
    ClientSocket socket;
    thread client_thread(&ClientSocket::peer2peer, &socket, ip, port);
    /*This creates a background thread that runs the peer2peer function, allowing the client to act as both:
    Client (main thread): Communicates with the tracker, sends commands
    Server (background thread): Listens for incoming connections from other peers
    */
   /*
   ─────────────────────────┐     ┌──────────────────────────┐
│   Main Thread           │     │   Background Thread      │
│                         │     │   (client_thread)        │
├─────────────────────────┤     ├──────────────────────────┤
│ 1. Read user commands   │     │ 1. Start server socket   │
│ 2. Connect to tracker   │     │ 2. listen() for peers    │
│ 3. Send: create_user    │     │ 3. accept() connections  │
│ 4. Send: login          │     │ 4. Handle file uploads   │
│ 5. Send: upload_file    │     │    from other peers      │
│ 6. Send: download_file  │     │                          │
│ 7. Connect to peer...   │     │ (Runs in parallel!)      │
│ 8. Download chunks...   │     │                          │
└─────────────────────────┘     └──────────────────────────┘
         ↕                                  ↕
    Tracker Server                    Other Peers*/


    client_thread.detach(); //new created thread runs independently and listens 
    //for peer connections
    CommandHandler handler;
    handler.run_client(ip, port); //main thread runs run client and talks with tracker

    return 0;
}