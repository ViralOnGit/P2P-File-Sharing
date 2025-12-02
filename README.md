Peer-to-Peer File Sharing System (Single Tracker Version)
📌 Overview

This project is a peer-to-peer (P2P) distributed file sharing system implemented using C/C++ with system calls only.
The system allows users to create accounts, join groups, share files, and download files from peers inside the same group using parallel downloading, piecewise SHA1 verification, and thread-based communication.

This implementation is based on the Assignment–3 specification of the P2P file-sharing architecture 

AOS_assignment_3_M24

 but modified to run on a single tracker instead of a synchronized multi-tracker setup.

🛠 Features Implemented
✔ 1. User & Session Management

Create new user accounts

Login with authentication

Automatic restoration of previously shared files upon login

Logout (stops all sharing temporarily)

✔ 2. Group Management

Create new groups

Join existing groups

Leave groups

Accept or reject join requests (owner only)

List all groups

List pending join requests (for owners)

✔ 3. File Sharing

Share a file inside a group

Computes full-file SHA1

Computes piecewise SHA1 (512 KB pieces)

Stop sharing a file

Stop sharing all files on logout

All shared files become available for other peers instantly

✔ 4. Parallel File Downloading (Core Part)

Retrieve list of peers sharing the required file from tracker

Communicate with peers to fetch available piece-map

Custom Piece Selection Algorithm implemented

Ensures downloading occurs from multiple peers wherever possible

Balances pieces to reduce bottleneck

Parallel download using multiple threads

As soon as a piece is downloaded, it becomes available for upload to others

Full-file SHA1 verification after download

Supports multiple concurrent downloads

✔ 5. Tracker Functionalities

Maintains user accounts, groups, files shared, and peer info

Handles join requests and file indexing

Provides peer list for requested files

Runs using information provided in tracker_info.txt

Operates as a single reliable tracker in this implementation

📁 Project Structure
<Roll_Number>_A3/
│
├── tracker/
│   ├── tracker.cpp
│   ├── Makefile
│   └── ...
│
└── client/
    ├── client.cpp
    ├── Makefile
    └── ...

🚀 How to Run
1. Compile

Go inside both directories and compile using:

make

2. Run the Tracker

Format:

./tracker tracker_info.txt tracker_no


Example:

./tracker tracker_info.txt 1


tracker_info.txt contains:

<TRACKER_IP> <TRACKER_PORT>

3. Run the Client

Format:

./client <IP>:<PORT> tracker_info.txt


Example:

./client 127.0.0.1:5000 tracker_info.txt

💡 Supported Client Commands
Command	Description
create_user <user_id> <passwd>	Create new account
login <user_id> <passwd>	Login
create_group <group_id>	Create new group
join_group <group_id>	Request to join group
leave_group <group_id>	Leave group
list_groups	List all groups
list_requests <group_id>	Owner: list join requests
accept_request <group_id> <user_id>	Owner: accept user
list_files <group_id>	List sharable files in group
upload_file <filepath> <group_id>	Share file in group
download_file <group_id> <filename> <dest_path>	Start parallel download
show_downloads	Show download status (D / C)
[D] [grp_id] filename	Downloading
[C] [grp_id] filename	Completed
stop_share <group_id> <file_name>	Stop sharing file
logout	Logout and stop all sharing
🧩 Piece Division & SHA1 Handling

Each file is divided into 512 KB fixed-size pieces (last piece may be smaller)

For each piece, a 20-byte SHA1 hash is computed

Final file hash = concatenation of all piece-hash prefixes (as per assignment spec) 

AOS_assignment_3_M24

Verification happens after full download to ensure data integrity

🧠 Custom Piece Selection Algorithm

Your algorithm includes:

Fetch piece availability from all peers

Prefer rarest pieces first (if implemented)

Ensure minimum 2 peers are used whenever possible

Distribute piece requests across peers to maximize download speed

Each piece download handled in a separate thread

📌 Assumptions

Single tracker is always reachable & running

Tracker stores all metadata in memory during runtime

Client uploads file info (piece hashes + full file hash) but does NOT upload actual file data

Download resumes only from the start (no persistent partial state saved)

Newly downloaded piece becomes shareable immediately

📝 How the System Works (Summary)

Client registers and logs in

Joins or creates a group

Shares files in the group

When a file is requested:

Tracker returns list of peers sharing that file

Client contacts peers to fetch piece maps

Downloads pieces in parallel from multiple peers

As pieces download, they become available to others

Final SHA1 integrity check done

Logout temporarily stops sharing

📚 References

Assignment Specification Document (AOS Assignment 3) 

AOS_assignment_3_M24

Socket Programming (TCP)

POSIX Threads

OpenSSL SHA1 API
