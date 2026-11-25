import socket
import datetime

# Bind to all available network interfaces on port 4210
UDP_IP = "0.0.0.0"
UDP_PORT = 4210

# Set up the UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for ESP32 data on UDP port {UDP_PORT}...")
print("Press Ctrl+C to stop.")

# Open a CSV file in 'append' mode so it never overwrites old data
with open("sensor_data_log.csv", "a") as file:
    while True:
        # Wait until a packet is received
        data, addr = sock.recvfrom(1024)

        # Decode the packet and get the current PC time
        message = data.decode('utf-8').strip()
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Format and save
        log_entry = f"{timestamp}, {message}"
        print(f"Received: {log_entry}")

        file.write(log_entry + "\n")
        file.flush()  # Force save to disk immediately