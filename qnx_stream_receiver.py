# File: qnx_stream_receiver.py
# PURPOSE: Run this script on your host PC (Linux/Windows).
# It listens for a TCP connection from the QNX C client, receives data,
# and displays it as a video stream.

import socket
import numpy as np
import cv2

# --- Configuration ---
HOST = '0.0.0.0'  # Listen on all available network interfaces
PORT = 12345      # The port the C client will connect to

# --- IMPORTANT: Camera Frame Configuration ---
# You MUST set these values to match the resolution of the camera on the QNX device.
# The C client sends raw pixel data, so we need to know the exact dimensions
# to reconstruct the image correctly.
FRAME_WIDTH = 640
FRAME_HEIGHT = 480
# For BGR8888, there are 4 bytes per pixel (B, G, R, and an unused Alpha/padding byte)
BYTES_PER_PIXEL = 4
FRAME_SIZE_BYTES = FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL

def run_server():
    """
    Sets up the server, listens for a connection, and handles incoming data.
    """
    # Create a TCP/IP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Allow the socket to be reused immediately after closing
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Bind the socket to the address and port
        s.bind((HOST, PORT))
        
        # Start listening for incoming connections
        s.listen()
        
        print(f"[Python Server] Listening for a connection on {HOST}:{PORT}...")
        
        # Wait for a connection from the QNX client
        conn, addr = s.accept()
        
        # A connection has been accepted
        with conn:
            print(f"[Python Server] Accepted connection from {addr}")
            
            is_first_message = True
            
            while True:
                try:
                    # --- Handle the initial "Hello World" test message ---
                    if is_first_message:
                        # Peek at the first chunk of data without consuming it from the buffer
                        first_chunk = conn.recv(1024, socket.MSG_PEEK)
                        
                        # The C string includes a null terminator ('\0')
                        if b"Hello from QNX!\0" in first_chunk:
                            print("\n--- HELLO WORLD TEST PASSED ---")
                            # Now, actually receive the message to remove it from the buffer
                            hello_data = conn.recv(len(b"Hello from QNX!\0"))
                            print(f"  Received message: \"{hello_data.decode('utf-8').strip()}\"")
                            print("---------------------------------\n")
                            print("[Python Server] Now waiting for camera frames...")
                        
                        is_first_message = False

                    # --- Receive and process a full video frame ---
                    full_frame_data = b''
                    bytes_received = 0
                    
                    # Loop to ensure we receive exactly one full frame's worth of data
                    while bytes_received < FRAME_SIZE_BYTES:
                        # Calculate how many more bytes we need
                        remaining_bytes = FRAME_SIZE_BYTES - bytes_received
                        # Receive up to 4096 bytes at a time
                        chunk = conn.recv(min(remaining_bytes, 4096))
                        
                        if not chunk:
                            # The connection was closed by the client
                            print("\n[Python Server] Client closed the connection.")
                            return

                        full_frame_data += chunk
                        bytes_received += len(chunk)

                    # --- Reconstruct and display the image ---
                    # Convert the raw bytes into a NumPy array
                    frame_array = np.frombuffer(full_frame_data, dtype=np.uint8)
                    
                    # Reshape the 1D array into a 2D image (Height, Width, Channels)
                    # The C client sends BGRX (4 channels), so we use 4 here.
                    image = frame_array.reshape((FRAME_HEIGHT, FRAME_WIDTH, BYTES_PER_PIXEL))

                    # Display the image in a window
                    cv2.imshow('QNX Camera Stream', image)

                    # Allow the window to update and check for the 'q' key to quit
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        print("[Python Server] 'q' key pressed, shutting down.")
                        break

                except ConnectionResetError:
                    print("\n[Python Server] Connection was forcibly closed by the client.")
                    break
                except Exception as e:
                    print(f"\n[Python Server] An error occurred: {e}")
                    break

    # Clean up and close the display window
    cv2.destroyAllWindows()
    print("[Python Server] Server shut down.")


if __name__ == "__main__":
    # You will need to have opencv-python and numpy installed on your PC:
    # pip install opencv-python numpy
    run_server()
