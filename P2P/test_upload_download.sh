#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== P2P File Sharing Test ===${NC}\n"

# Step 1: Start Tracker
echo -e "${GREEN}Step 1: Starting Tracker on 127.0.0.1:8070${NC}"
cd Tracker
./tracker 127.0.0.1:8070 tracker_info.txt > tracker.log 2>&1 &
TRACKER_PID=$!
echo "Tracker started with PID: $TRACKER_PID"
sleep 2

# Check if tracker is running
if ! ps -p $TRACKER_PID > /dev/null; then
    echo -e "${RED}ERROR: Tracker failed to start!${NC}"
    cat tracker.log
    exit 1
fi
echo -e "${GREEN}✓ Tracker is running${NC}\n"

cd ..

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up processes...${NC}"
    pkill -P $TRACKER_PID 2>/dev/null
    kill $TRACKER_PID 2>/dev/null
    echo -e "${GREEN}Done!${NC}"
}

trap cleanup EXIT

echo -e "${YELLOW}=== Test Instructions ===${NC}"
echo -e "Now manually test the following:"
echo -e ""
echo -e "${GREEN}Terminal 1 (Alice - Uploader):${NC}"
echo -e "  cd Client"
echo -e "  ./client 127.0.0.1:5001 tracker_info.txt"
echo -e "  create_user alice pass123"
echo -e "  login alice pass123"
echo -e "  create_group 1"
echo -e "  upload_file AOS_assignment_3_M24\\ \\(1\\).pdf 1"
echo -e ""
echo -e "${GREEN}Terminal 2 (Bob - Downloader):${NC}"
echo -e "  cd Client"
echo -e "  ./client 127.0.0.1:5002 tracker_info.txt"
echo -e "  create_user bob pass456"
echo -e "  login bob pass456"
echo -e "  join_group 1"
echo -e ""
echo -e "${GREEN}Terminal 1 (Alice accepts Bob):${NC}"
echo -e "  list_requests 1"
echo -e "  accept_request 1 bob"
echo -e ""
echo -e "${GREEN}Terminal 2 (Bob downloads):${NC}"
echo -e "  download_file 1 AOS_assignment_3_M24\\ \\(1\\).pdf /tmp/downloaded_assignment.pdf"
echo -e ""
echo -e "${YELLOW}Press Ctrl+C when done testing${NC}"

# Keep script running
wait
