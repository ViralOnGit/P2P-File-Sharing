P2P INTERIM+ FINAL SUBMISSION

Implemented Commands :

Tracker : Run Tracker: ./tracker tracker_info.txt tracker_no
tracker_info.txt - Contains ip , port details of one tracker (as of now)

Client:        (way to run)
1)Run Client: ./client <IP>:<PORT> tracker_info.txt
tracker_info.txt - Contains ip, port details of all the trackers

2)Create User Account: create_user <user_id> <passwd>
-> Only allowed unique user_ids

3)Login: login <user_id> <passwd>
-> Password and user_id integrity will be checked.

4)Create Group: create_group <group_id>
->only allowed after user logs in
->no 2 groups with same name allowed

5)Join Group: join_group <group_id>
->only allowed after user logs in
-> group exists or not is checked.
-> only allowed if client is not the owner of group.

6)List pending join: list_requests <group_id>
->only allowed after user logs in
-> group exists or not is checked.
-> only allowed if client is the owner of group.

7)List All Group In Network: list_groups
->only allowed after user logs in
->list all groups exists in the network.

8)Accept Group Joining Request: accept_request <group_id> <user_id>
-> only group owner can accept the request of the user
-> removes request from joining_request map
-> add the user_id into group <group_id>

9)Leave Group: leave_group <group_id>
-> owner is allowed to leave if there's some other member than owner
-> all other members are allowed to leave anytime
-> checks if the requestor firstly part of group or not.

10)List All sharable Files In Group: list_files <group_id>
->if uploaded client gets logged out or left group or stop_shares then those files doesn't get displayed.

11)Upload File: upload_file <file_path> <group_id>
->gives the informations regarding files to trackers!

12)Logout: logout
->same as traditional logout feature.

13)Stop sharing: stop_share <group_id> <file_name>
-> stop_sharing that perticular file in that perticular group
-> verify it by follow up of list_files <group_id>



Compile:

Client: g++ client.cpp -o client -lssl -lcrypto -o client
run client: ./client 127.0.0.1:8040 tracker_info.txt
Tracker: g++ tracker.cpp -o tracker
run tracker: ./tracker tracker_info.txt 1


create_user a 1
login a 1
create_group 123
upload_file R.mp4 123

*b side*
create_user b 1
login b 1
join_group 123

*a side*
accept_request 123 b

*b side* 
download_file 123 R.mp4 d125.mp4

stop_share 123 R.mp4

*c side*
create_user c 1
login c 1
join_group 123

*a side*
accept_request 123 c

*c side*
download_file 123 R.mp4 Dkddhhdfk.mp4