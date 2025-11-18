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
    server_addr.sin_family = AF_INET; //socket will use IPv4
    server_addr.sin_port = htons(port);//converts port number to network byte order which is required
    //for network communication
    inet_pton(AF_INET, ip_address.c_str(), &server_addr.sin_addr);
}

TrackerSocket::~TrackerSocket() {
    if (server_socket >= 0) 
    {
        close(server_socket); /*Closes the listening socket
                                No new clients can connect
                                Any blocked accept() call returns with an error*/
    }
    for (auto& t : client_threads) 
    {
        if (t.joinable()) /*because there could be case that when we attempt to
        close the server, there may be some threads performing some tasks e.g here
        accepting connection, so they must be done with their work before completing*/
            t.join();
    }
}

bool TrackerSocket::init() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0); //we are creating TCP Sockets here
    //sock stream means TCP sockets which ensures
    //reliable delivery, sock_datagram is also a type of socket which is UDP which doesnt guarantee
    //reliable delivery, packets can get lost but if they are reached then they will be error free
    //sock datagram can be used if packet dropping is not much issue i.e video calling etc
    if (server_socket < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
    { //used for reusing the port and address when tracker restarts quickly to 
        //avoid "Address already in use" errors
        perror("setsockopt");
        close(server_socket);
        return false;
    }

    // Bind socket
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {//it binds the socket to the specified IP address and port number
         //so that it can listen for incoming connections on that address and port
         //basically it associates the socket with the specified IP and port
        cerr << "Bind failed." << std::endl;
        close(server_socket);
        return false;
    }

    // Listen on socket
    if (listen(server_socket, 5) < 0) //starts listening for incoming connections on this socket
    //maximum of 5 pending connections can be queued if simultaneous connection requests arrive
    {

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
        //Calls accept() to accept an incoming connection from the listening socket (server_socket):
        //It blocks until a client connects.
        //Returns a new socket descriptor (client_socket) for communicating with that specific client.
        //Fills client_addr with the client's address and updates addr_len if needed.
        //If successful, client_socket is a valid file descriptor; if not, it's -1.
        if (client_socket < 0) {
            cerr << "Accept failed." << std::endl;
            continue;
        }
        cout << "Client and Tracker Connected!" << std::endl;
        client_threads.push_back(thread(handle_ClientFunctions, client_socket));
        /*by above line a new thread is created for the accepted client
        connection, which runs handle_clientfunc for specifically
        that client and likewise create seperate threads for 
        communicating with each client.
        New thread is created to handle that client
        Thread object is stored in client_threads vector

        Main thread immediately loops back to accept() for the next client
        Meanwhile, the new thread is busy handling the first client*/
        /*
        Process Memory Space
        ┌─────────────────────────────────────┐
        │  Code Segment (shared)              │
        │  - handle_ClientFunctions()         │
        │  - TrackerCommandHandler code       │
        ├─────────────────────────────────────┤
        │  Data Segment (shared)              │
        │  - commandHandler (GLOBAL)          │ ⚠️ Multiple threads access this!
        │  - client_threads vector            │
        ├─────────────────────────────────────┤
        │  Heap (shared)                      │
        │  - Dynamic allocations              │
        ├─────────────────────────────────────┤
        │  Thread 1 Stack                     │
        │  - buffer[20480]                    │
        │  - tokenise_commands                │
        │  - client_socket (copy)             │
        ├─────────────────────────────────────┤
        │  Thread 2 Stack                     │
        │  - buffer[20480]                    │
        │  - tokenise_commands                │
        │  - client_socket (copy)             │
        ├─────────────────────────────────────┤
        │  Thread 3 Stack                     │
        │  - buffer[20480]                    │
        │  - ...                              │
        └─────────────────────────────────────┘
                */
    }
}