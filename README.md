# Video Streaming from QNX 8.0 to a Host PC Using WSL
---
This guide is for developers using WSL as their primary environment to build and deploy applications for QNX 8.0. It provides a solution for streaming video from a QNX device to the host PC for real-time processing.

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

---

## Core Setup & Configuration

### Configure the Windows Firewall

You must create a firewall rule on your Windows host to accept the incoming connection from the QNX device.

Open PowerShell as an Administrator.

Run the following command:

```powershell
New-NetFirewallRule -DisplayName "Allow QNX Camera Stream" -Direction Inbound -Protocol TCP -LocalPort 12345 -Action Allow
```

---

### Find your Windows Host IP Address

The QNX device needs to connect to your Windows host, not WSL. Run ipconfig in Command Prompt or PowerShell to find your computer's IP address.

### Set the Server IP

Edit camera_streamer.c to change the `SERVER_IP` macro to the Windows IP address you just found.

---

### Connection & Data Protocol
This solution uses a standard TCP socket stream for communication. The C client on QNX utilizes the native libsocket library to send a continuous stream of raw, uncompressed 640x480 BGR8888 pixel data. 

The Python server on Windows then uses the standard socket library to receive this data for processing with OpenCV

### Run the System

Start the Python Server: Move the Python script to the native Windows filesystem and run the qnx_stream_receiver.py script. 

```powershell
python qnx_stream_receiver.py
```
It will begin listening for a connection.

Run the QNX Client: From your WSL terminal, transfer the compiled camera_streamer binary to your QNX device (e.g., via scp) and execute it.

A new window managed by OpenCV should now appear on your Windows desktop, displaying the live video stream.

### Why the Server Must Run on Windows

The Python server script must be run from the native Windows filesystem (e.g., C:\) to ensure network reliability. Running it from within the WSL filesystem fails because WSL2's isolated virtual network makes it inaccessible to other devices on the LAN. Workarounds like netsh port forwarding are notoriously unreliable and can lead to silent connection failures. Bypassing the WSL networking stack entirely by running the server natively on Windows is the solution used in this project.

