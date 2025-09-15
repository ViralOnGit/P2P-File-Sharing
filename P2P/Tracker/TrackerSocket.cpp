#include "TrackerSocket.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
using namespace std;

// External function for handling clients (declared here; defined in tracker.cpp)
extern void handle_ClientFunctions(int client_socket);

TrackerSocket::TrackerSocket(const std::string& ip, int port) 
    : server_socket(-1), ip_address(ip), port(port) {
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr);
}

TrackerSocket::~TrackerSocket() {
    if (server_socket >= 0) {
        close(server_socket);
    }
    for (auto& t : client_threads) {
        if (t.joinable()) t.join();
    }
}

bool TrackerSocket::init() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_socket);
        return false;
    }

    // Bind socket
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed." << std::endl;
        close(server_socket);
        return false;
    }

    // Listen on socket
    if (listen(server_socket, 5) < 0) {
        cerr << "Listen failed." << std::endl;
        close(server_socket);
        return false;
    }

    cout << "Tracker is running on " << ip_address << ":" << port << std::endl;
    return true;
}

void TrackerSocket::run() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "Accept failed." << std::endl;
            continue;
        }
        cout << "Client and Tracker Connected!" << std::endl;
        client_threads.push_back(thread(handle_ClientFunctions, client_socket));
    }
}