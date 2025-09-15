#ifndef TRACKER_SERVER_H
#define TRACKER_SERVER_H

#include <vector>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;

class TrackerSocket {
private:
    int server_socket;
    string ip_address;
    int port;
    std::vector<std::thread> client_threads;
    sockaddr_in server_addr;

public:
    TrackerSocket(const std::string& ip, int port);
    ~TrackerSocket();

    bool init();
    void run();
};

#endif // TRACKER_SERVER_H