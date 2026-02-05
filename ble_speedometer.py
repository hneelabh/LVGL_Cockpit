import asyncio
from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions
)
import socket

# This is a standard UUID for "Speed"
SPEED_UUID = "00002A67-0000-1000-8000-00805F9B34FB"

# Setup the IPC (Inter-Process Communication) to talk to C
C_SOCKET_FILE = "/tmp/lvgl_speed.sock"

def speed_written_callback(characteristic, value):
    """This runs whenever your phone sends a new speed value"""
    # 'value' is already the raw bytes sent from your phone!
    speed = int.from_bytes(value, byteorder='little')
    print(f"📱 Phone sent: {speed} km/h")

    # Send this number to the C program
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        sock.sendto(speed.to_bytes(4, 'little'), C_SOCKET_FILE)
    except Exception as e:
        print(f"Socket error: {e}")

async def run():
    # 1. Name the device
    server = BlessServer("Visteon RPi4B")
    
    # 2. Add the Speed Characteristic (Writeable)
    await server.add_new_service("00001818-0000-1000-8000-00805f9b34fb") # Cycling Power Service
    await server.add_new_characteristic(
        "00001818-0000-1000-8000-00805f9b34fb",
        SPEED_UUID,
        GATTCharacteristicProperties.write,
        b'\x00',
        GATTAttributePermissions.writeable
    )

    # 3. Start listening
    server.write_request_func = speed_written_callback
    await server.start()
    print("🏍️ LVGL_Motors BLE Server running. Connect with nRF App!")
    
    await asyncio.sleep(999999) # Keep running

asyncio.run(run())