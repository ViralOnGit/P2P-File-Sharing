#!/usr/bin/env python3
import os, shlex, shutil, subprocess, time
from pathlib import Path

# --- Paths ---
REPO = Path(__file__).resolve().parent
TRACKER_DIR = REPO / "Tracker"
CLIENT_DIR  = REPO / "Client"

TRACKER_BIN = "tracker"   # produced in Tracker/
CLIENT_BIN  = "client"    # produced in Client/

# --- Client instances (title, ip:port) ---
CLIENTS = [
    ("a", "127.0.0.1:8040"),
    ("b", "127.0.0.1:8041"),
    ("c", "127.0.0.1:8042"),
]

SESSION = "p2p"  # tmux session name

def need(tool):
    if not shutil.which(tool):
        raise SystemExit(f"[!] '{tool}' not found. Install it (e.g., sudo apt install {tool}).")

def sh(cmd, **kw):
    return subprocess.run(["bash","-lc",cmd], check=True, **kw)

def compile_all():
    print("[*] compiling tracker (with utils)...")
    sh(f"cd {shlex.quote(str(TRACKER_DIR))} && g++ tracker.cpp utils.cpp -o {TRACKER_BIN}")
    print("[+] tracker built -> Tracker/" + TRACKER_BIN)

    print("[*] compiling client (with OpenSSL)...")
    sh(f"cd {shlex.quote(str(CLIENT_DIR))} && g++ client.cpp -o {CLIENT_BIN} -lssl -lcrypto")
    print("[+] client built -> Client/" + CLIENT_BIN)

def build_tmux_script():
    tdir = shlex.quote(str(TRACKER_DIR))
    cdir = shlex.quote(str(CLIENT_DIR))

    # pane layout:
    #  pane 0: tracker (top-left)
    #  pane 1: client-a (top-right)
    #  pane 2: client-b (bottom-right)
    #  pane 3: client-c (bottom-left)

    a_addr = CLIENTS[0][1]
    b_addr = CLIENTS[1][1]
    c_addr = CLIENTS[2][1]

    tracker_cmd = f'./{TRACKER_BIN} tracker_info.txt 1'
    a_cmd = f'./{CLIENT_BIN} {a_addr} tracker_info.txt'
    b_cmd = f'./{CLIENT_BIN} {b_addr} tracker_info.txt'
    c_cmd = f'./{CLIENT_BIN} {c_addr} tracker_info.txt'

    lines = [
        # fresh session
        f"tmux kill-session -t {SESSION} 2>/dev/null || true",

        # create panes & start processes (set working dirs with -c)
        f"tmux new-session -d -s {SESSION} -c {tdir} 'bash -lc {shlex.quote(tracker_cmd)}'",
        f"tmux split-window -h   -c {cdir} 'bash -lc {shlex.quote(a_cmd)}'",
        f"tmux split-window -v   -c {cdir} 'bash -lc {shlex.quote(b_cmd)}'",
        f"tmux select-pane -t 0",
        f"tmux split-window -v   -c {cdir} 'bash -lc {shlex.quote(c_cmd)}'",
        f"tmux select-layout tiled",
        "sleep 1",

        # --- scripted warmup sequence ---

        # client a
        f"tmux send-keys -t {SESSION}:0.1 'create_user a 1' Enter",
        f"tmux send-keys -t {SESSION}:0.1 'login a 1' Enter",
        f"tmux send-keys -t {SESSION}:0.1 'create_group 123' Enter",
        f"tmux send-keys -t {SESSION}:0.1 'upload_file R.mp4 123' Enter",
        "sleep 1",

        # client b
        f"tmux send-keys -t {SESSION}:0.2 'create_user b 1' Enter",
        f"tmux send-keys -t {SESSION}:0.2 'login b 1' Enter",
        f"tmux send-keys -t {SESSION}:0.2 'join_group 123' Enter",
        "sleep 1",

        # a accepts b
        f"tmux send-keys -t {SESSION}:0.1 'accept_request 123 b' Enter",
        "sleep 1",

        # b downloads then stops sharing
        f"tmux send-keys -t {SESSION}:0.2 'download_file 123 R.mp4 d12345.mp4' Enter",
        f"tmux send-keys -t {SESSION}:0.2 'stop_share 123 R.mp4' Enter",
        "sleep 1",

        # client c
        f"tmux send-keys -t {SESSION}:0.3 'create_user c 1' Enter",
        f"tmux send-keys -t {SESSION}:0.3 'login c 1' Enter",
        f"tmux send-keys -t {SESSION}:0.3 'join_group 123' Enter",
        "sleep 1",

        # a accepts c
        f"tmux send-keys -t {SESSION}:0.1 'accept_request 123 c' Enter",
        "sleep 1",

        # c downloads
        f"tmux send-keys -t {SESSION}:0.3 'download_file 123 R.mp4 Dkddhhdfk.mp4' Enter",

        # attach for manual testing
        f"tmux attach -t {SESSION}"
    ]

    # run the tmux script inside a Terminator window
    script = " ; ".join(lines)
    term_cmd = f"terminator -e 'bash -lc {shlex.quote(script)}'"
    return term_cmd

def main():
    # sanity checks
    for p in [TRACKER_DIR/"tracker.cpp", TRACKER_DIR/"utils.cpp", TRACKER_DIR/"utils.h",
              TRACKER_DIR/"tracker_info.txt", CLIENT_DIR/"client.cpp", CLIENT_DIR/"tracker_info.txt"]:
        if not p.exists():
            raise SystemExit(f"[!] missing: {p}")

    need("terminator")
    need("tmux")

    # optional: warn if file to upload absent
    if not (CLIENT_DIR/"R.mp4").exists():
        print("[!] Note: Client/R.mp4 not found — upload_file will fail unless you add it.")

    compile_all()

    cmd = build_tmux_script()
    print("[*] launching terminator + tmux panes...")
    subprocess.Popen(cmd, shell=True)

if __name__ == "__main__":
    main()
