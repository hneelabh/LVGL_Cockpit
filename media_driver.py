import socket
import os
import subprocess

# Simple Listener
CMD_SOCKET = "/tmp/lvgl_cmd.sock"

if os.path.exists(CMD_SOCKET):
    os.remove(CMD_SOCKET)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
sock.bind(CMD_SOCKET)

print("🎧 Listening for UI commands...")

while True:
    data, addr = sock.recvfrom(1024)
    cmd = data.decode('utf-8')
    print(f"Received: {cmd}")
    
    # EXECUTE REAL COMMANDS
    if cmd == "NEXT":
        # Simulate Keypress or use MPC/DBUS
        os.system("playerctl next") 
    elif cmd == "PREV":
        os.system("playerctl previous")
    elif cmd == "PLAYPAUSE":
        os.system("playerctl play-pause")