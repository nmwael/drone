#!/usr/bin/env python3
"""
SRT Proxy: ESP32 (on WiFi) -> Host -> 172.160.27.93:8890

Run on HOST machine:
    python3 srt_proxy.py

This creates a UDP proxy that:
- Listens on 0.0.0.0:8890 (for ESP32)
- Forwards all packets to 172.160.27.93:8890
- Returns responses back to ESP32
"""
import socket
import threading
import struct
import sys

LISTEN_IP = "0.0.0.0"
LISTEN_PORT = 8890
TARGET_IP = "172.160.27.93"
TARGET_PORT = 8890

class SRTProxy:
    def __init__(self):
        self.client_sock = None  # Will store ESP32's address
        self.target_addr = (TARGET_IP, TARGET_PORT)
        self.running = True
        
    def start(self):
        # Listen socket for ESP32
        self.listen_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listen_sock.bind((LISTEN_IP, LISTEN_PORT))
        
        # Outbound socket to target
        self.out_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.out_sock.settimeout(2.0)
        
        print(f"=== SRT Proxy Started ===")
        print(f"Listening on: {LISTEN_IP}:{LISTEN_PORT}")
        print(f"Forwarding to: {TARGET_IP}:{TARGET_PORT}")
        print(f"Waiting for ESP32...")
        
        # Start forwarding thread (target -> ESP32)
        forward_thread = threading.Thread(target=self.forward_responses, daemon=True)
        forward_thread.start()
        
        # Main loop: ESP32 -> target
        self.main_loop()
    
    def main_loop(self):
        while self.running:
            try:
                data, addr = self.listen_sock.recvfrom(65536)
                
                # Remember ESP32's address
                if self.client_sock is None:
                    self.client_sock = addr
                    print(f"ESP32 connected from: {addr}")
                
                # Forward to target
                if addr == self.client_sock:
                    self.out_sock.sendto(data, self.target_addr)
                    if len(data) > 4 and (data[0] & 0x80):
                        # Control packet - log it
                        cdt = struct.unpack('>I', data[:4])[0]
                        stype = struct.unpack('>I', data[4:8])[0]
                        print(f"  -> ESP32->Target: {len(data)} bytes, CDT=0x{cdt:08x}, Type=0x{stype:08x}")
                    else:
                        print(f"  -> ESP32->Target: {len(data)} bytes (DATA)")
                        
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"Error: {e}")
    
    def forward_responses(self):
        while self.running:
            try:
                data, addr = self.out_sock.recvfrom(65536)
                
                # Forward back to ESP32
                if self.client_sock:
                    self.listen_sock.sendto(data, self.client_sock)
                    if len(data) > 4 and (data[0] & 0x80):
                        cdt = struct.unpack('>I', data[:4])[0]
                        stype = struct.unpack('>I', data[4:8])[0]
                        print(f"  <- Target->ESP32: {len(data)} bytes, CDT=0x{cdt:08x}, Type=0x{stype:08x}")
                    else:
                        print(f"  <- Target->ESP32: {len(data)} bytes (DATA)")
                        
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"Forward error: {e}")
    
    def stop(self):
        self.running = False
        if self.listen_sock:
            self.listen_sock.close()
        if self.out_sock:
            self.out_sock.close()

if __name__ == "__main__":
    proxy = SRTProxy()
    try:
        proxy.start()
    except KeyboardInterrupt:
        print("\nShutting down...")
        proxy.stop()