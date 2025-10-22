#ifndef FILETRANSFERHANDLER_H
#define FILETRANSFERHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include "clientutils.h"

using namespace std;

class FileTransferHandler {
private:
    ClientUtils utils;
    
public:
    FileTransferHandler();
    
    // Upload file handling
    void handle_upload_file(
        int client_socket,
        const string& command,
        const string& current_loggedin,
        const string& ip,
        int port
    );
    
    // Download file handling
    void handle_download_file(
        int client_socket,
        const string& command,
        int port
    );
    
    // Stop share handling
    void handle_stop_share(const string& command);
    
    // Download helper functions (used by handle_download_file)
    static bool download_file_from_peer(
        int peer_socket, 
        string groupid, 
        int dest_fd, 
        const string& filename, 
        int chunk_index, 
        size_t chunk_size, 
        const string& expected_sha1
    );
    
    static void download_worker(
        int client_socket,
        int dest_fd,
        const string& filename,
        const string& groupid,
        const string& port,
        const vector<string>& fourth_vector,
        vector<ChunkStatus>& chunk_status,
        mutex& chunk_mutex,
        vector<pair<int, vector<string>>>& chunk_ports_vec,
        atomic<int>& next_chunk_index
    );
};

#endif // FILETRANSFERHANDLER_H
