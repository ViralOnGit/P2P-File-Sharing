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
    client_thread.detach();
    CommandHandler handler;
    handler.run_client(ip, port);

    return 0;
}