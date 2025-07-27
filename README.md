# High-Performance Video Streaming from QNX 8.0 to a Host PC
---
This guide is for developers using WSL as their primary environment to build and deploy applications for QNX 8.0. It provides a robust, low-latency solution for streaming video from a QNX device (e.g., Raspberry Pi 4) to the host PC for real-time processing.

## A Note on WSL Networking

Running a network server inside WSL that needs to accept connections from external LAN devices is unreliable.

Testing shows that standard methods like `netsh` port forwarding can lead to silent connection failures where packets are dropped. Therefore, the only proven method for this project is to run the Python server directly on the Windows host. Your WSL terminal is still used for compiling, file transfers, and SSH.

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

### Run the System

This step requires running the Python server from your native Windows filesystem to ensure network reliability.

1. Move the Server Script to Windows
The Python script must be run from outside the WSL filesystem and run:
```powershell
python qnx_stream_receiver.py
```
The server is now listening.

#### Run the Client (from QNX)

From your WSL terminal, use `scp` to transfer the compiled `camera_streamer` binary to your QNX device.

SSH into your QNX device from WSL and execute the binary.

A new window managed by OpenCV should now appear on your Windows desktop, displaying the live video stream.

