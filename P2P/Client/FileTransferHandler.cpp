#include "FileTransferHandler.h"
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <random>

#define SHA_DIGEST_LENGTH 20
#define CHUNK_SIZE 524288 // 512 KB

extern std::map<std::string, std::set<std::string>> downloadedFiles;
extern std::map<std::pair<std::string, std::string>, int> stopShared;
extern std::mutex tracker_comm_mutex;

FileTransferHandler::FileTransferHandler() {}

// Download helper function - downloads a single chunk from a peer
bool FileTransferHandler::download_file_from_peer(int peer_socket, string groupid, int dest_fd, const std::string& filename, int chunk_index, size_t chunk_size, const std::string& expected_sha1) 
{ //Do FROM HERE!!!
    ClientUtils utils;
    std::string request = filename + " " + groupid + " " + std::to_string(chunk_index) + "\n";
    write(peer_socket, request.c_str(), request.size());

    uint32_t chunk_size_net;
    ssize_t size_read = read(peer_socket, &chunk_size_net, sizeof(chunk_size_net));
    if (size_read != sizeof(chunk_size_net)) 
    {
        std::cerr << "Failed to read chunk size." << std::endl;
        close(peer_socket);
        return false;
    }
    uint32_t actual_chunk_size = ntohl(chunk_size_net);

    char file_chunk[CHUNK_SIZE];
    size_t total_read = 0;
    while (total_read < actual_chunk_size) {
        ssize_t bytes_received = read(peer_socket, file_chunk + total_read, actual_chunk_size - total_read);
        if (bytes_received <= 0) {
            std::cerr << "Connection closed or error while reading chunk data." << std::endl;
            close(peer_socket);
            return false;
        }
        total_read += bytes_received;
    }

    std::string chunk_sha1 = utils.sha1_of_buffer(file_chunk, actual_chunk_size);
    if (chunk_sha1 != expected_sha1) {
        std::cerr << "Chunk " << chunk_index << " hash mismatch! Expected: " << expected_sha1 << ", Got: " << chunk_sha1 << std::endl;
        close(peer_socket);
        return false;
    }

    off_t offset = static_cast<off_t>(chunk_index) * CHUNK_SIZE;
    if (pwrite(dest_fd, file_chunk, actual_chunk_size, offset) < 0)
    {
        std::cerr << "Error writing chunk " << chunk_index << " to file." << std::endl;
        close(peer_socket);
        return false;
    }

    std::cout << "Chunk " << chunk_index << " downloaded and verified." << std::endl;
    close(peer_socket);
    return true;
}

// Download worker function - handles multi-threaded chunk downloading
void FileTransferHandler::download_worker(
    int client_socket, int dest_fd, const string& filename, const string& groupid, 
    const string& port, const vector<string>& allsha1s, vector<ChunkStatus>& chunk_status, 
    mutex& chunk_mutex, vector<pair<int, vector<string>>>& chunk_ports_vec, 
    std::atomic<int>& next_chunk_index) {

    while (true) {
        int current_index = next_chunk_index.fetch_add(1);
        if (current_index >= static_cast<int>(chunk_ports_vec.size())) break;
        int chunk_to_download = chunk_ports_vec[current_index].first;

        {
            std::lock_guard lock(chunk_mutex);
            if (chunk_status[chunk_to_download] == ChunkStatus::Done) continue;
            chunk_status[chunk_to_download] = ChunkStatus::InProgress;
        }

        cout << "Thread " << std::this_thread::get_id() << ": Downloading chunk " << chunk_to_download << endl;

        bool chunk_downloaded = false;

        vector<string> peer_ports = chunk_ports_vec[current_index].second;  // Direct copy (efficient for small vectors).
        for (const string& i : peer_ports) {
            cout << "Peer port: " << i << " ";
        }
        cout << endl;

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(peer_ports.begin(), peer_ports.end(), g);

        for (const auto& peer_port : peer_ports) {
            int peer_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (peer_socket < 0) continue;

            struct sockaddr_in peer_addr;
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            peer_addr.sin_port = htons(stoi(peer_port));

            if (connect(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
                close(peer_socket);
                continue;
            }

            cout << "Connected to peer at port: " << peer_port << endl;

            if (download_file_from_peer(peer_socket, groupid, dest_fd, filename, chunk_to_download, 
                                        CHUNK_SIZE, allsha1s[chunk_to_download])) {
                {
                    std::lock_guard status_lock(chunk_mutex);
                    chunk_status[chunk_to_download] = ChunkStatus::Done;
                }

                {
                    std::lock_guard comm_lock(tracker_comm_mutex); //as thread will communicate to tracker
                    //to share the update that it has downloaded a chunk so mutex help in thread synchronization
                    //with tracker

                    string have_chunk_msg = "have_chunk;" + filename + ";" + to_string(chunk_to_download) + ";" + port + ";";
                    write(client_socket, have_chunk_msg.c_str(), have_chunk_msg.size());
                    char ack_buffer[32];
                    int ack_bytes = read(client_socket, ack_buffer, sizeof(ack_buffer) - 1);
                    if (ack_bytes > 0) {
                        string ack(ack_buffer, ack_bytes);  // Safer than manual null-term.
                        cout << "Tracker response: " << ack << endl;
                    } else if (ack_bytes < 0) {
                        cerr << "Error reading ACK from tracker: " << strerror(errno) << endl;
                    }
                }

                chunk_downloaded = true;
                close(peer_socket);
                break;  // Success: Stop retrying other peers for this chunk.
            }
            close(peer_socket);  // Failure: Retry next peer.
        }

        if (!chunk_downloaded) {
            std::lock_guard lock(chunk_mutex);
            chunk_status[chunk_to_download] = ChunkStatus::Missing;
        }
    }
}

void FileTransferHandler::handle_upload_file(
    int client_socket,
    const string& command,
    const string& current_loggedin,
    const string& ip,
    int port
)
{
    char copied_command[256];
    vector<string> upload_tokens;
    vector<string> chunk_SHAs;
    strncpy(copied_command, command.c_str(), sizeof(copied_command));
    copied_command[sizeof(copied_command) - 1] = '\0';

    upload_tokens = utils.get_tokens_for_command(copied_command, " \t");

    string filepath = upload_tokens[1];
    string filepath_str(filepath);

    string groupid = upload_tokens[2];

    size_t last_slash = filepath_str.find_last_of('/');
    string filename = filepath_str.substr(last_slash + 1);

    struct stat stat_buf;
    size_t filesize = 0;
    if (stat(filepath.c_str(), &stat_buf) == 0)
    {
        filesize = stat_buf.st_size;
    }
    else
    {
        std::cerr << "Error getting file size" << std::endl;
        return;
    }

    string fileSHA1 = utils.get_file_sha1(filename.c_str());
    vector<string> chunkHashes = utils.hash_chunks(filename.c_str());

    cout << "chunk hashes size" << chunkHashes.size() << endl;

    string message = "upload_file " + filename + " " + groupid + " " + to_string(filesize) + " " + fileSHA1 + " " + current_loggedin + " " + filepath + " " + ip + " " + to_string(port) + " ";
    for (const auto &chunk_sha1 : chunkHashes)
    {
        message += chunk_sha1 + " ";
    }
    message += "\n";

    write(client_socket, message.c_str(), message.size());

    char buffer[1024];
    int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        cout << "Response from tracker: " << buffer << endl;
    }
}
/*
    from the command received from client, it parses the command 
    to extract necessary details like filename, destination path, group ID, etc.
    then get full file SHA and each chunk SHA to send to tracker as any client
    who will download the file should have this info to compare for integrity.
    and likewise create messsage with all this info and sends to tracker.
*/

void FileTransferHandler::handle_download_file(
    int client_socket,
    const string& command,
    int port
)
{
    char copied_command[256];
    vector<string> download_command_tokens;
    strncpy(copied_command, command.c_str(), sizeof(copied_command));
    copied_command[sizeof(copied_command) - 1] = '\0';
    download_command_tokens = utils.get_tokens_for_command(copied_command, " \t");
    string filename = download_command_tokens[2];//know why its extracting
    string destination_path = download_command_tokens[3];
    string groupid = download_command_tokens[1];
    if(downloadedFiles[groupid].find(filename) != downloadedFiles[groupid].end())
    {
        cout << "File already downloaded in this group." << endl;
        return;
    }

    write(client_socket, command.c_str(), command.size()); //see what is client socket
    char buffer[20480];
    int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    vector<string> download_tokens = utils.get_tokens_for_command(buffer, ";"); //1st is all sha1s of file then seperation
    //by ; and then each chunk number space its available ports(space seperated) ;(; seperated)
    //then new chunk no and its ports in that order
    vector<string> allsha1s;

    if (!download_tokens.empty()) //storing all sha1s of file in allsha1s vector
    { 
        char temp[20480];
        strncpy(temp, download_tokens[0].c_str(), sizeof(temp));
        temp[sizeof(temp) - 1] = '\0';
        char *subtoken = strtok(temp, " ");
        while (subtoken != nullptr) {
            allsha1s.push_back(string(subtoken));
            subtoken = strtok(nullptr, " ");
        }
    }

    vector<pair<int, vector<string>>> chunk_ports_vec;
    if (download_tokens.size() > 1) //now storing chunk number and all ports that have it
    {
        for (int i = 1; i < download_tokens.size(); ++i) 
        {
            char temp[20480];
            strncpy(temp, download_tokens[i].c_str(), sizeof(temp));
            temp[sizeof(temp) - 1] = '\0';
            vector<string> tokens;
            char *subtoken = strtok(temp, " "); //strtok returns a ptr to next token
            //of the string
            if (subtoken == nullptr) continue;
            int chunk_no = stoi(subtoken);
            subtoken = strtok(nullptr, " ");
            while (subtoken != nullptr) 
            {
                tokens.push_back(string(subtoken));
                subtoken = strtok(nullptr, " ");
            }
            chunk_ports_vec.push_back({chunk_no, tokens});
        }
    }

    sort(chunk_ports_vec.begin(), chunk_ports_vec.end(),
        [](const pair<int, vector<string>>& a, const pair<int, vector<string>>& b) 
        {
            return a.second.size() < b.second.size();
        }); //RAREST FIRST!!

    vector<ChunkStatus> chunk_status(allsha1s.size(), ChunkStatus::Missing);
    std::mutex chunk_mutex; //know why chunk mutex here

    int dest_fd = open(destination_path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (dest_fd < 0) 
    {
        cerr << "Error opening destination file: " << destination_path << endl;
        return;
    }
    std::atomic<int> next_chunk_index(0);
    /*
    Why atomic (not just int):
    // BAD (non-atomic):
    int next_chunk = next_chunk_index; // Thread 1 reads 0
    next_chunk_index++;                // Thread 2 reads 0 (before Thread 1 increments!)
    // Both get chunk 0!

    // GOOD (atomic):
    int next_chunk = next_chunk_index.fetch_add(1); 
    // Thread 1 gets 0, Thread 2 gets 1 (guaranteed)
        */
    int NUM_THREADS = 4;
    vector<thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) //creating 4 threads to download chunks in parallel
    {
        threads.emplace_back(
            download_worker,
            client_socket,
            dest_fd,
            filename,
            groupid,
            to_string(port),
            allsha1s,
            std::ref(chunk_status),
            std::ref(chunk_mutex),
            std::ref(chunk_ports_vec),
            std::ref(next_chunk_index)
        );
    }
    /* SAME TASK running in parallel, but on DIFFERENT DATA
Thread 1:           Thread 2:           Thread 3:           Thread 4:
- Download chunk 0  - Download chunk 1  - Download chunk 2  - Download chunk 3
- Download chunk 4  - Download chunk 5  - Download chunk 6  - Download chunk 7
- Download chunk 8  - ...// */
/*Without Multithreading (Sequential):
    Main Thread Downloads:
├─ Connect to Peer A, download chunk 0 (2 seconds)
├─ Connect to Peer B, download chunk 1 (2 seconds)
├─ Connect to Peer C, download chunk 2 (2 seconds)
├─ Connect to Peer D, download chunk 3 (2 seconds)
└─ Total time: 8 seconds
    With 4 Threads (Parallel):
    Thread 1: Connect to Peer A, download chunk 0 (2 seconds)
Thread 2: Connect to Peer B, download chunk 1 (2 seconds)  } ALL AT
Thread 3: Connect to Peer C, download chunk 2 (2 seconds)  } THE SAME
Thread 4: Connect to Peer D, download chunk 3 (2 seconds)  } TIME!

Total time: 2 seconds (4x faster!)

*/
    for (auto& t : threads) t.join();
    /*
        What it does:
        Main thread blocks here until ALL 4 worker threads finish
        Ensures all chunks are downloaded before proceeding
    */

    int cnt=0;
    for (const auto& status : chunk_status) 
    {
        if (status == ChunkStatus::Missing) {
            cnt++;
            break;
        }
    }
    if(cnt>0)
    {
        cout << "Some chunks are missing." << endl;
        string nd = "NOT_DONE";
        write(client_socket, nd.c_str(), nd.length());
    }
    else{
        cout << "File is Downloaded Successfully." << endl;
        string done_msg = "DONE";
        downloadedFiles[groupid].insert(filename);

        write(client_socket, done_msg.c_str(), done_msg.size());
    }
    close(dest_fd);
}

void FileTransferHandler::handle_stop_share(const string& command)
{
    ClientUtils utils;
    char copied_command[256];
    vector<string> shared_tokens;
    strncpy(copied_command, command.c_str(), sizeof(copied_command));
    shared_tokens = utils.get_tokens_for_command(copied_command, " \t");
    string filename = shared_tokens[2];
    string groupid = shared_tokens[1];
    stopShared[{filename, groupid}] = 1;
}
