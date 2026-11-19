#!/usr/bin/env python3
"""
Simple SRT listener test for host machine.
Run on host: python3 test_srt_host.py
"""
import socket
import struct

def test_srt_handshake():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', 8890))
    sock.settimeout(10)
    
    print("SRT listener started on 0.0.0.0:8890")
    print("Waiting for ESP32 handshake...")
    
    try:
        # Receive induction handshake
        data, addr = sock.recvfrom(1024)
        print(f"Received {len(data)} bytes from {addr}")
        print(f"  CDT: {data[:4].hex()}")
        print(f"  Type: {data[4:8].hex()}")
        print(f"  Dst SockID: {data[8:12].hex()}")
        print(f"  Version: {data[12:16].hex()}")
        print(f"  Max Payload: {data[16:20].hex()}")
        print(f"  Ext Flags: {data[20:24].hex()}")
        
        if len(data) > 24 and data[20:24] != b'\x00\x00\x00\x00':
            # Parse SID extension
            pos = 24
            while pos < len(data):
                if pos + 4 <= len(data):
                    ext_type = struct.unpack('>H', data[pos:pos+2])[0]
                    ext_len = struct.unpack('>H', data[pos+2:pos+4])[0]
                    if ext_type == 0x0005:  # SRT_CMD_SID
                        sid = data[pos+4:pos+4+ext_len].decode()
                        print(f"  Stream ID: {sid}")
                    pos += 4 + ext_len
                    # Pad to 4
                    while pos % 4:
                        pos += 1
        
        # Send handshake response (HSRESP)
        our_sockid = 0x12345678
        resp = struct.pack('>IIIII', 0x80000000, 0x00000001, our_sockid, 0x00000405, 1316)
        resp += struct.pack('>I', 0)  # no extensions
        sock.sendto(resp, addr)
        print(f"Sent HSRESP to {addr}")
        
        # Wait for conclusion handshake
        data, addr = sock.recvfrom(1024)
        print(f"Received conclusion HS from {addr}: {len(data)} bytes")
        
        # Send conclusion response
        resp = struct.pack('>IIIII', 0x80000000, 0x00000001, our_sockid, 0x00000405, 1316)
        resp += struct.pack('>I', 0)
        sock.sendto(resp, addr)
        print("Handshake complete! Connection established.")
        
        # Now receive DATA packets
        print("\nWaiting for TS data packets...")
        while True:
            data, addr = sock.recvfrom(2048)
            if len(data) >= 4 and (data[0] & 0x80) == 0:
                # DATA packet
                print(f"Received TS data: {len(data)} bytes from {addr}")
            else:
                print(f"Received control: {len(data)} bytes from {addr}")
                
    except socket.timeout:
        print("Timeout - no handshake received")
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        sock.close()

if __name__ == '__main__':
    test_srt_handshake()