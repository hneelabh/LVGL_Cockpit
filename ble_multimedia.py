import asyncio
import socket
from bless import BlessServer, BlessGATTCharacteristic, GATTCharacteristicProperties, GATTAttributePermissions
from pydbus import SystemBus
from gi.repository import GLib

# --- CONFIGURATION ---
SPEED_SOCKET = "/tmp/lvgl_speed.sock"
MUSIC_SOCKET = "/tmp/lvgl_music.sock"
SPEED_UUID = "00002A67-0000-1000-8000-00805F9B34FB"

# --- HELPER: Send Data to C ---
def send_to_socket(socket_path, data):
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        sock.sendto(data, socket_path)
    except Exception:
        pass # Socket might not be open yet

# --- PART 1: BLE SPEEDOMETER (The Sensor) ---
def speed_written_cb(characteristic, value):
    try:
        # User sends a single byte (0-255) or integer
        speed = int.from_bytes(value, byteorder='little')
        print(f"🏎️ Speed Received: {speed}")
        send_to_socket(SPEED_SOCKET, speed.to_bytes(4, 'little'))
    except Exception as e:
        print(f"Error parsing speed: {e}")

# --- PART 2: MUSIC METADATA (The Media Player) ---
def check_media_metadata():
    """Polls BlueZ for the currently playing song + Time + Status"""
    try:
        bus = SystemBus()
        manager = bus.get("org.bluez", "/")
        managed_objects = manager.GetManagedObjects()
        
        for path, interfaces in managed_objects.items():
            if "org.bluez.MediaPlayer1" in interfaces:
                player = bus.get("org.bluez", path)
                
                # 1. Track Info
                try:
                    track = player.Track
                except Exception:
                    continue 
                
                title = track.get("Title", "Unknown Title")
                artist = track.get("Artist", "Unknown Artist")
                album = track.get("Album", "") 
                
                # 2. Duration (ms -> sec)
                duration_ms = track.get("Duration", 0)
                duration_sec = int(duration_ms / 1000)

                # 3. Position (ms -> sec)
                try:
                    position_ms = player.Position
                    position_sec = int(position_ms / 1000)
                except Exception:
                    position_sec = 0

                # 4. Status (Playing/Paused/Stopped) - CRITICAL FIX
                try:
                    # BlueZ returns "Playing", "Paused", "Stopped"
                    status_str = player.Status
                except Exception:
                    status_str = "Paused"

                # 5. Format: 6 items (Title|Artist|Album|Dur|Pos|Status)
                info_string = f"{title}|{artist}|{album}|{duration_sec}|{position_sec}|{status_str}"
                
                # Debug print to verify
                print(f"🎵 Sending: {info_string}")
                
                send_to_socket(MUSIC_SOCKET, info_string.encode('utf-8'))
                return
                
    except Exception as e:
        pass
    
    return True

# --- MAIN LOOP ---
async def run():
    print("🚀 Starting Hybrid Server (BLE + Music)...")
    
    # 1. Start BLE Server
    server = BlessServer("LVGL_Cockpit_Pro")
    server.read_request_func = lambda c: b'\x00' # Simple read handler
    server.write_request_func = speed_written_cb
    
    # Add Service & Speed Characteristic
    await server.add_new_service("00001818-0000-1000-8000-00805f9b34fb")
    await server.add_new_characteristic(
        "00001818-0000-1000-8000-00805f9b34fb",
        SPEED_UUID,
        GATTCharacteristicProperties.write | GATTCharacteristicProperties.read,
        b'\x00',
        GATTAttributePermissions.writeable | GATTAttributePermissions.readable
    )
    
    await server.start()
    print("✅ BLE Advertising as 'LVGL_Cockpit_Pro'")
    print("✅ Listening for DBus Media updates...")

    # 2. Loop forever checking music
    while True:
        check_media_metadata()
        await asyncio.sleep(1) # Check music every 1 second

# --- EXECUTION ---
if __name__ == "__main__":
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        loop.run_until_complete(run())
    except KeyboardInterrupt:
        print("\nStopping server...")