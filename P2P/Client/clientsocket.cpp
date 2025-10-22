#include "clientsocket.h"
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <map>
#include <set>

#define SHA_DIGEST_LENGTH 20
#define CHUNK_SIZE 524288 // 512 KB

extern std::map<std::string, std::set<std::string>> downloadedFiles;
extern std::map<std::pair<std::string, std::string>, int> stopShared;
extern int check_login;

using namespace std;

void ClientSocket::clienttoseeder(int client_socket) {
    char buffer[1024];
    if(check_login == 0) {
        close(client_socket);
        return;
    }
    int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';
    istringstream iss(buffer);
    string filename;
    string groupid;
    int chunk_index;
    iss >> filename >> groupid >> chunk_index;

    if(stopShared[{filename, groupid}] == 1)
    {
        close(client_socket);
        return;
    }

    int file_fd = open(filename.c_str(), O_RDONLY);
    if (file_fd < 0) {
        close(client_socket);
        return;
    }

    off_t offset = static_cast<off_t>(chunk_index) * CHUNK_SIZE;
    char file_chunk[CHUNK_SIZE];
    ssize_t bytes_read = pread(file_fd, file_chunk, CHUNK_SIZE, offset);

    uint32_t chunk_size = (bytes_read > 0) ? static_cast<uint32_t>(bytes_read) : 0;
    uint32_t chunk_size_net = htonl(chunk_size);
    write(client_socket, &chunk_size_net, sizeof(chunk_size_net));

    if (bytes_read > 0) {
        write(client_socket, file_chunk, bytes_read);
    }

    close(file_fd);
    close(client_socket);
}

void ClientSocket::peer2peer(string server_ip, int server_port)
{
    int opt = 1;

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        cout << "Socket not created" << endl;
        return;
    }
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Socket not modified");
        close(server_socket_fd);
        return;
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &(server_addr.sin_addr));
    if (bind(server_socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
         perror("Bind failed.");
        close(server_socket_fd);
        return;
    }
    if (listen(server_socket_fd, 5) < 0) {
        cerr << "Listen failed." << endl;
        close(server_socket_fd);
        return ;
    }

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket_fd, (sockaddr*)&client_addr, &addr_len);
        cout << "Connection Established Peer to Peer" << std::endl;
        thread client_thread(&ClientSocket::clienttoseeder, this, client_socket);
        client_thread.detach();
    }

    close(server_socket_fd);
}