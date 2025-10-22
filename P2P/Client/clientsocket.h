#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <string>

using namespace std;

class ClientSocket {
public:
    void clienttoseeder(int client_socket);
    void peer2peer(string server_ip, int server_port);
};

#endif // CLIENTSOCKET_H