#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <openssl/evp.h>  // For EVP API
#include <sys/stat.h>
#include<map>
#include<set>
#include<thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>    // For open(), O_WRONLY, O_CREAT
#include <unordered_set>
#include <vector>
#include <random>
#include <algorithm>
#include<mutex>
#include<atomic>
#define SHA_DIGEST_LENGTH 20
#define CHUNK_SIZE 524288 // 512 KB


using namespace std;
enum class ChunkStatus { Missing, InProgress, Done };//this and below
std::mutex tracker_comm_mutex;


vector<string> get_tokens_for_command(char buffer[1024],const char *delimiter)
{
    vector<string> tokens;
        char *token = strtok(buffer, delimiter); 
        while (token != nullptr) 
        {
            tokens.push_back(string(token)); 
            token = strtok(nullptr, delimiter);
        }
    return tokens;
}

vector<string> hash_chunks(const char *filename) {
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << "Error opening file for reading." << endl;
        return {};
    }

    unsigned char buf[CHUNK_SIZE]; // Buffer for chunks
    vector<string> chunkHashes;
    size_t totalBytesRead = 0;
    size_t bytesRead;
    int chunkCount = 0;
    while ((bytesRead = fread(buf, 1, CHUNK_SIZE, pf)) > 0) 
    {
        // Calculate SHA-1 for the current chunk
        totalBytesRead += bytesRead;
        chunkCount++;

        EVP_MD_CTX *chunkCtx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(chunkCtx, EVP_sha1(), nullptr);
        EVP_DigestUpdate(chunkCtx, buf, bytesRead);

        unsigned char chunkDigest[SHA_DIGEST_LENGTH];
        EVP_DigestFinal_ex(chunkCtx, chunkDigest, nullptr);

        // Convert chunk digest to hex string
        char chunkHash[SHA_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            sprintf(&chunkHash[i * 2], "%02x", chunkDigest[i]);
        }

        if (totalBytesRead > 0) 
        {
            chunkHashes.push_back(string(chunkHash)); // Store the chunk hash
        } // Store the chunk hash

        EVP_MD_CTX_free(chunkCtx); // Free the chunk context
    }
    fclose(pf);
    return chunkHashes;
}

int shaforcompletefile(const char *filename, unsigned char *out) {
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << "Error opening file for reading." << endl;
        return -1;
    }

    unsigned char buf[CHUNK_SIZE]; // Buffer for chunks

    // SHA-1 context for the entire file
    EVP_MD_CTX *fileCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(fileCtx, EVP_sha1(), nullptr);

    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, CHUNK_SIZE, pf)) > 0) {
        // Update the overall SHA-1 digest with the current chunk
        EVP_DigestUpdate(fileCtx, buf, bytesRead);
    }

    // Finalize the overall SHA-1 digest
    EVP_DigestFinal_ex(fileCtx, out, nullptr);
    EVP_MD_CTX_free(fileCtx); // Free the file context
    fclose(pf); // Close the file

    return 0;
}

string get_file_sha1(const char *filename) {
    unsigned char fileDigest[SHA_DIGEST_LENGTH];
    if (shaforcompletefile(filename, fileDigest) != 0) {
        return "$"; // Error handling
    }

    // Convert the file SHA-1 to a hex string
    char fileSHA1[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&fileSHA1[i * 2], "%02x", fileDigest[i]);
    }
    return string(fileSHA1);
}
void clienttoseeder(int client_socket) {
    char buffer[1024];
    int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';
    istringstream iss(buffer);
    string filename;
    int chunk_index;
    iss >> filename >> chunk_index;

    //check stop sharing is true or not 

    int file_fd = open(filename.c_str(), O_RDONLY);
    if (file_fd < 0) {
        close(client_socket);
        return;
    }

    off_t offset = static_cast<off_t>(chunk_index) * CHUNK_SIZE;
    char file_chunk[CHUNK_SIZE];
    ssize_t bytes_read = pread(file_fd, file_chunk, CHUNK_SIZE, offset);

    // Send the chunk size first (as 4 bytes, network byte order)
    uint32_t chunk_size = (bytes_read > 0) ? static_cast<uint32_t>(bytes_read) : 0;
    uint32_t chunk_size_net = htonl(chunk_size);
    write(client_socket, &chunk_size_net, sizeof(chunk_size_net));

    // Send the chunk data
    if (bytes_read > 0) {
        write(client_socket, file_chunk, bytes_read);
    }

    close(file_fd);
    close(client_socket);
}

void peer2peer(string server_ip,int server_port)//client behaving as server who will send the file if it has the chunk/file
 {
    int opt = 1;


    // Socket creation
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
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());  // Convert IP to binary
    server_addr.sin_port = htons(server_port);  // Convert port to network byte order
    inet_pton(AF_INET, server_ip.c_str(), &(server_addr.sin_addr));
    if (bind(server_socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
         perror("Bind failed.");  // More detailed error message
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
        thread client_thread(clienttoseeder, client_socket);
        client_thread.detach();
    }

    close(server_socket_fd);  // Close the server socket when done
}

// Helper to compute SHA1 hash of a buffer
std::string sha1_of_buffer(const char* buf, size_t len) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(ctx, buf, len);
    EVP_DigestFinal_ex(ctx, hash, nullptr);
    EVP_MD_CTX_free(ctx);

    char hash_str[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        sprintf(&hash_str[i * 2], "%02x", hash[i]);
    hash_str[SHA_DIGEST_LENGTH * 2] = '\0';
    return std::string(hash_str);
}

bool download_file_from_peer(int peer_socket, int dest_fd, const std::string& filename, int chunk_index, size_t chunk_size, const std::string& expected_sha1) {
    // Send request: "<filename> <chunk_index>\n"
    std::string request = filename + " " + std::to_string(chunk_index) + "\n";
    write(peer_socket, request.c_str(), request.size());

    // Read the 4-byte chunk size first
    uint32_t chunk_size_net;
    ssize_t size_read = read(peer_socket, &chunk_size_net, sizeof(chunk_size_net));
    if (size_read != sizeof(chunk_size_net)) {
        std::cerr << "Failed to read chunk size." << std::endl;
        close(peer_socket);
        return false;
    }
    uint32_t actual_chunk_size = ntohl(chunk_size_net);

    // Now read the chunk data
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

    // Verify SHA1
    std::string chunk_sha1 = sha1_of_buffer(file_chunk, actual_chunk_size);
    if (chunk_sha1 != expected_sha1) {
        std::cerr << "Chunk " << chunk_index << " hash mismatch! Expected: " << expected_sha1 << ", Got: " << chunk_sha1 << std::endl;
        close(peer_socket);
        return false;
    }

    // Write chunk to the correct position in the file
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

int get_next_missing_chunk(std::vector<ChunkStatus>& chunk_status, std::mutex& chunk_mutex) {//this
    std::lock_guard<std::mutex> lock(chunk_mutex);

    // Create a vector of all possible indices
    std::vector<int> indices(chunk_status.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Shuffle the indices for randomness
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    // Find the first Missing chunk in random order
    for (int idx : indices) {
        if (chunk_status[idx] == ChunkStatus::Missing) {
            chunk_status[idx] = ChunkStatus::InProgress;
            return idx;
        }
    }
    return -1; // No missing chunks left
}


void download_worker(//this
    int client_socket,
    int dest_fd,
    const string& filename,
    const string& groupid,
    const string& port,
    const vector<string>& fourth_vector,
    vector<ChunkStatus>& chunk_status,
    mutex& chunk_mutex,
    vector<pair<int, vector<string>>>& chunk_ports_vec,
    std::atomic<int>& next_chunk_index)
{
    while (true)  
    {
        // 1. Pick a missing chunk in a thread-safe way
        int current_index = next_chunk_index.fetch_add(1);
        if (current_index >= chunk_ports_vec.size()) break; 
        // cout << "Thread " << std::this_thread::get_id() << ": Downloading chunk " 
        //      << chunk_to_download << endl;
        int chunk_to_download = chunk_ports_vec[current_index].first;
        {
            std::lock_guard<std::mutex> lock(chunk_mutex);
            if (chunk_status[chunk_to_download] == ChunkStatus::Done) continue;
            chunk_status[chunk_to_download] = ChunkStatus::InProgress;
        } 
                cout << "Thread " << std::this_thread::get_id() << ": Downloading chunk " 
             << chunk_to_download << endl;


        bool chunk_downloaded = false;        
        std::lock_guard<std::mutex> lock(tracker_comm_mutex);
        // --- (b) Parse peer ports ---
        vector<string> peer_ports;
        for(const auto& port : chunk_ports_vec[current_index].second) 
        {
            peer_ports.push_back(port);
        }
        for(string i : peer_ports) 
        {
            cout << "Peer port: " << i <<"";
        }
        cout << endl;
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(peer_ports.begin(), peer_ports.end(), g);
        
        // --- (c) Try to download from one of the peers ---
        for (const auto& peer_port : peer_ports) 
        {
            int peer_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (peer_socket < 0) continue;
            struct sockaddr_in peer_addr;
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // or use actual IP
            peer_addr.sin_port = htons(stoi(peer_port));
            if (connect(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
                close(peer_socket);
                continue;
            }
            cout << "Connected to peer at port: " << peer_port << endl;

            if (download_file_from_peer(peer_socket, dest_fd, filename, chunk_to_download, CHUNK_SIZE, fourth_vector[chunk_to_download])) 
            {
            
                std::lock_guard<std::mutex> lock(chunk_mutex);
                chunk_status[chunk_to_download] = ChunkStatus::Done;
            

                // --- (d) Announce to tracker after download ---
                // std::lock_guard<std::mutex> lock(tracker_comm_mutex);

                string have_chunk_msg = "have_chunk;" + filename + ";" + to_string(chunk_to_download) + ";" + port + ";";
                char ack_buffer[32];
                write(client_socket, have_chunk_msg.c_str(), have_chunk_msg.size());
                int ack_bytes = read(client_socket, ack_buffer, sizeof(ack_buffer) - 1);
                if (ack_bytes > 0) {
                    ack_buffer[ack_bytes] = '\0';
                    cout << "Tracker response: " << ack_buffer << endl;
                }
                chunk_downloaded = true;
                close(peer_socket);
                break;
            }
            close(peer_socket);
        }

        // --- (e) If download failed, mark as missing for retry ---
        if (!chunk_downloaded) 
        {
            std::lock_guard<std::mutex> lock(chunk_mutex);
            chunk_status[chunk_to_download] = ChunkStatus::Missing;
        }

    } // end for loop
}



unordered_set<string> st = {"quit", "login", "create_group", "join_group", "list_groups", "accept_request", "list_requests", "leave_group", "logout", "upload_file", "stop_share", "download_file", "create_user"}; // Set of valid commands


void run_client(const string& ip, int port) 
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) 
    {
        cerr << "Failed to create socket." << endl;
        return;
    }
    vector<string> tokens;
    FILE *file = fopen("tracker_info.txt", "r");
    if (file == nullptr) 
    {
        cerr << "Error: Unable to open file." << endl;
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)) 
    {
        // Strip the newline character if it exists
        buffer[strcspn(buffer, "\n")] = 0;
        char *token = strtok(buffer, ":"); 
        while (token != nullptr) 
        {
            tokens.push_back(string(token)); 
            token = strtok(nullptr, ":");
        }
    }
    fclose(file);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    // cout<<port<<endl;
    server_addr.sin_port = htons(stoi(tokens[1]));


    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        cerr << "Connection failed." << endl;
        close(client_socket);
        return;
    }

    cout << "Connected to tracker at " << ip << ":" << stoi(tokens[1]) << endl;
    int check_login=0;
    string current_loggedin="";
    while (true) 
    {
        string command;
        cout << "Enter command (or 'quit' to exit): ";
        getline(cin, command);
        // cout<<"check login"<<check_login<<endl;

        string compare_command="";
        string a=" ";

        string filepath="";//upload_file filepath
        string filename="";//upload_file filename
        string groupid="";//upload_file groupid
        string message="";//for upload_file message sending to tracker which's different from other commands
        for(int i=0;i<command.size();i++)
        {
            if(command[i]==' ')
            {
                break;
            }
            compare_command+=command[i];
        }
        if(compare_command=="login" && check_login==0)
        {
            char copied_command[256];
            vector<string> login_tokens;
            
            strncpy(copied_command, command.c_str(), sizeof(copied_command));
            copied_command[sizeof(copied_command) - 1] = '\0'; // Ensure null-termination
            login_tokens=get_tokens_for_command(copied_command," \t");
            a=login_tokens[1];
        }
        
        if(compare_command=="login" && check_login==1)
        {
            cout<<"other user is already logged in"<<endl;
            continue;
        }
        else if(compare_command=="create_group" && check_login==0)
        {
            // cout<<check_login<<endl;
            cout<<"First login/Register before creating group"<<endl;
            continue;
        }
        else if(compare_command=="create_group" && check_login==1)
        {
            // cout<<check_login<<endl;
            cout<<current_loggedin<<endl;
            
        }
        else if(compare_command=="join_group" && check_login==0)
        {
            cout<<"First login/Register before making request to join"<<endl;
            continue;
        }
        else if(compare_command=="list_groups" && check_login==0)
        {
            cout<<"First login/Register before making request to join"<<endl;
            continue;
        }
        else if(compare_command=="accept_request" && check_login==0)
        {
            cout<<"First login/Register before making request to join"<<endl;
            continue;
        }
        else if(compare_command=="list_requests" && check_login==0)
        {
            cout<<"First login/Register before checking list requests"<<endl;
            continue;
        }
        else if(compare_command=="leave_group" && check_login==0)
        {
            cout<<"First login/Register to leave group"<<endl;
            continue;
        }
        else if(compare_command=="logout" && check_login==0)
        {
            cout<<"You are not logged in"<<endl;
            continue;
        }
        else if(compare_command=="upload_file" && check_login==0)
        {
            cout<<"You are not logged in"<<endl;
            continue;
        }
        else if(compare_command=="stop_sharing" && check_login==0)
        {
            cout<<"You are not logged in"<<endl;
            continue;
        }
        else if(compare_command=="upload_file" && check_login==1)
        {

            ////In short in upload file we are sending 1)filename 2)groupId 3)filesize 4)whole file SHA1  5)user's id 7)file path 8)IP 9)port 10)each chunk SHA1
            char copied_command[256];
            vector<string> upload_tokens;
            vector<string> chunk_SHAs;
            strncpy(copied_command, command.c_str(), sizeof(copied_command));
            copied_command[sizeof(copied_command) - 1] = '\0'; // Ensure null-termination


            upload_tokens=get_tokens_for_command(copied_command," \t"); // Tokenize the command


            filepath=upload_tokens[1]; 
            string filepath_str(filepath);

            groupid=upload_tokens[2];

            size_t last_slash = filepath_str.find_last_of('/');
            filename = filepath_str.substr(last_slash + 1); // as file name will be like this C:/Viral/Singham.mp4

            struct stat stat_buf;
            size_t filesize=0;
            if (stat(filepath.c_str(), &stat_buf) == 0) 
            {
                 filesize=stat_buf.st_size; // File size in bytes
            } 
            else 
            {
                std::cerr << "Error getting file size" << std::endl;
                continue;
            }

            string fileSHA1 = get_file_sha1(filename.c_str());//whole file's SHA
            vector<string> chunkHashes = hash_chunks(filename.c_str());//each chunk's SHA

            cout<<"chunk hashes size"<<chunkHashes.size()<<endl;    

            string message = "upload_file " + filename + " " + groupid + " " + to_string(filesize) + " " + fileSHA1 + " " + current_loggedin + " " +filepath + " " + ip + " " + to_string(port) + " ";
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
                buffer[bytes_received] = '\0'; // Null-terminate the string
                cout << "Response from tracker: " << buffer << endl;
            }               
            continue;

         // cout<<current_loggedin<<endl;
        }
        else if(compare_command=="create_user")
        {

        }
        else if(compare_command=="download_file" && check_login==0)
        {
            cout<<"You are not logged in"<<endl;
            continue;
        }
        else if(compare_command == "download_file" && check_login == 1)//this
        {
            // 1. Parse command and get filename, destination path
            char copied_command[256];
            vector<string> download_command_tokens;
            strncpy(copied_command, command.c_str(), sizeof(copied_command));
            copied_command[sizeof(copied_command) - 1] = '\0';
            download_command_tokens = get_tokens_for_command(copied_command, " \t");
            string filename = download_command_tokens[2];
            string destination_path = download_command_tokens[3];
            string groupid = download_command_tokens[1];   // <-- FIXED


            // 2. Ask tracker for file metadata (chunk SHA1s, etc.)
            write(client_socket, command.c_str(), command.size());
            char buffer[20480];
            int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
             //it will receive all chunk_SHA1s and thehunk no and its ports
            //so create map and store all the ports according to the chunk
            //1) seperate via tokens 2) create a map with chunk_no as key and ports as values
            //---------------DONE------------------//
            
            

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                // cout << "Response from tracker: " << buffer << endl;
            }
            vector<string> download_tokens = get_tokens_for_command(buffer, ";");
            vector<string> fourth_vector; // chunk SHA1s

            // Parse chunk SHA1s (first token)
            if (!download_tokens.empty()) {
                char temp[20480];
                strncpy(temp, download_tokens[0].c_str(), sizeof(temp));
                temp[sizeof(temp) - 1] = '\0';
                char *subtoken = strtok(temp, " ");
                while (subtoken != nullptr) {
                    fourth_vector.push_back(string(subtoken));
                    subtoken = strtok(nullptr, " ");
                }
            }

            // Parse chunk-to-ports mapping (new format)
            vector<pair<int, vector<string>>> chunk_ports_vec;
            if (download_tokens.size() > 1) {
                for (size_t i = 1; i < download_tokens.size(); ++i) {
                    char temp[20480];
                    strncpy(temp, download_tokens[i].c_str(), sizeof(temp));
                    temp[sizeof(temp) - 1] = '\0';
                    vector<string> tokens;
                    char *subtoken = strtok(temp, " ");
                    if (subtoken == nullptr) continue;
                    int chunk_no = stoi(subtoken);
                    subtoken = strtok(nullptr, " ");
                    while (subtoken != nullptr) {
                        tokens.push_back(string(subtoken));
                        subtoken = strtok(nullptr, " ");
                    }
                    chunk_ports_vec.push_back({chunk_no, tokens});
                }
            }

            // Sort by rarity (lowest number of ports first)
            sort(chunk_ports_vec.begin(), chunk_ports_vec.end(),
                [](const pair<int, vector<string>>& a, const pair<int, vector<string>>& b) {
                    return a.second.size() < b.second.size();
                });

            // 3. Prepare download state
            //now we have to connect to the ports by rarest first vector 
            //then download that chunk and mark it as done
            //3) after downloading send have_chunk message to tracker
            //4) repeat 1-3 until all chunks are downloaded
            //----------------DONE------------------//


            vector<ChunkStatus> chunk_status(fourth_vector.size(), ChunkStatus::Missing);
            std::mutex chunk_mutex;

            int dest_fd = open(destination_path.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            if (dest_fd < 0) {
                cerr << "Error opening destination file: " << destination_path << endl;
                return;
            }
            std::atomic<int> next_chunk_index(0);  
            const int NUM_THREADS = 4; 
            vector<thread> threads;
            for (int i = 0; i < NUM_THREADS; ++i) {
                threads.emplace_back(
                    download_worker,
                    client_socket,       
                    dest_fd,
                    filename,
                    groupid,
                    to_string(port),
                    fourth_vector,
                    std::ref(chunk_status),
                    std::ref(chunk_mutex), 
                    std::ref(chunk_ports_vec) ,
                    std::ref(next_chunk_index) // Pass the chunk-to-ports mapping
                );
            }

            for (auto& t : threads) t.join();


            // After all threads join
            cout << "File is Downloaded Successfully." << endl;
            string done_msg = "DONE";
            write(client_socket, done_msg.c_str(), done_msg.size());
            close(dest_fd);

            
        }

        else if(compare_command=="list_files" && check_login==0)
        {
            cout<<"You are not logged in"<<endl;
            continue;
        }
        else if (st.find(compare_command) == st.end()) 
        {
            cout << "Invalid command. Please try again." << endl;
            continue;
        }
        if(compare_command!="upload_file" && compare_command!="download_file")
        {
            write(client_socket, command.c_str(), command.size());

        // Receive the response from the tracker
            char buffer[20480];
            int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
            if (bytes_received > 0) 
            {
                buffer[bytes_received] = '\0'; // Null-terminate the string
                cout << "Response from tracker: " << buffer << endl;
                // cout<<buffer<<endl;
                if (strcmp(buffer, "Logged in successfully") == 0)
                {
                    current_loggedin+=a;
                    check_login = 1;
                }
                if (strcmp(buffer, "Logged out successfully") == 0) 
                {
                    check_login = 0;  // Mark the user as logged out on the client side
                    current_loggedin=" ";
                }
                // cout<<"now check login"<<check_login<<endl;
            } 
            else 
            {
                cerr << "Failed to receive response from tracker." << endl;
                break;
            }
            if (command == "quit") 
            {
                // cout<<"hii"<<endl;
                check_login = 0;
                break;
            }
        }
        
    }
        

// Close the client socket
close(client_socket);

}

int main(int argc,char* argv[]) {
    if (argc!=3) 
    {
        cerr<<"Arguments must be 3 in initialising the client"<<endl;
        return 1;
    }

     // Get the IP and port from argv[1]
    char* ip_port = argv[1];
    char* tracker_info_file = argv[2]; // The second argument is the tracker info file

    // Tokenize ip_port to separate IP and Port using colon ":"
    char* ip = strtok(ip_port, ":");
    char* port_str = strtok(nullptr, ":");

    if (ip == nullptr || port_str == nullptr) {
        cerr << "Error: Invalid format for <IP>:<PORT>. Expected format is <IP>:<PORT>." << endl;
        return 1;
    }

    // Convert port from string to integer
    int port = atoi(port_str);

    // Print extracted IP and port for debugging
    cout << "IP Address: " << ip << endl;
    cout << "Port: " << port << endl;
    thread client_thread(peer2peer,ip,port);
    client_thread.detach();
    run_client(ip,port);

    return 0;
}
