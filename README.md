# Video Streaming from QNX 8.0 to a Host PC Using WSL
---
This guide provides a solution for streaming video from a Pi Camera on a QNX-powered Raspberry Pi to a host PC, tailored for developers using WSL as their primary environment for the QNX 8.0.

---

## Prerequisites

### Host PC Environment

- Windows 10 or 11 with WSL2 installed.
- Python 3 installed on Windows (for running the server).
- Required Python packages for the Windows installation are numpy and opencv
  
### QNX System Requirements

QNX SDP 8.0 is required.

Install the following packages from the QNX Software Center:

- com.qnx.qnx800.target.sf.camapi  
- com.qnx.qnx800.target.sf.base  
- com.qnx.qnx800.target.mm.aoi  
- com.qnx.qnx800.target.mm.mmf.core
  
## Core Setup & Configuration

### Important Note
The provided client configuration is specifically for the official Raspberry Pi Camera Module. Using different camera hardware will require modifications to the C client (camera_streamer.c) to handle other resolutions or pixel formats. Additionally, to enable the correct driver for the Pi Camera, you must first edit the system's startup script.

On the QNX device, log in as the root user (using su with the password root) and edit the startup file at /system/etc/post_startup.sh. In the camera section of this file, find the line for the "Camera Module 3" (line 91) and remove its comment (#). Then, add a comment to disable the simulated camera (line 103). After saving the file, reboot the device using the shutdown command. The physical camera will then be active and available to applications.

### Configure the Windows Firewall

You must create a firewall rule on your Windows host to accept the incoming connection from the QNX device.

Open PowerShell as an Administrator.

Run the following command:

```powershell
New-NetFirewallRule -DisplayName "Allow QNX Camera Stream" -Direction Inbound -Protocol TCP -LocalPort 12345 -Action Allow
```


### Find your Windows Host IP Address

The QNX device needs to connect to your Windows host, not WSL. Run ipconfig in Command Prompt or PowerShell to find your computer's IP address.

### Set the Server IP

Edit camera_streamer.c to change the `SERVER_IP` macro to the Windows IP address you just found.


### Connection & Data Protocol
This solution uses a standard TCP socket stream for communication. The C client on QNX utilizes the native libsocket library to send a continuous stream of BGR8888 pixel data. 

The Python server on Windows then uses the standard socket library to receive this data for processing with OpenCV

### Run the System
Move the Python script to the native Windows filesystem and run the qnx_stream_receiver.py script. 
```powershell
python qnx_stream_receiver.py
```
It will begin listening for a connection.

From your WSL terminal, transfer the compiled camera_streamer binary to your QNX device (e.g., via scp) and execute it.

A new window managed by OpenCV should now appear on your Windows desktop, displaying the live video stream.

### Why the Server Must Run on Windows

The Python server script must be run from the native Windows filesystem to ensure network reliability. Running it from within the WSL filesystem fails because WSL2's isolated virtual network makes it inaccessible to other devices on the LAN. Workarounds like netsh port forwarding are were unreliable and can lead to silent connection failures. Bypassing the WSL networking stack entirely by running the server natively on Windows is the solution used in this project.

