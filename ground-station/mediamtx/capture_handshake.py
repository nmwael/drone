#!/usr/bin/env python3
"""
Capture real SRT handshake: run MediaMTX as listener, ffmpeg as caller.
This shows the actual wire format SRT servers expect.
"""

import subprocess
import threading
import time
import socket
import struct
import sys

# Use port 18890 to avoid conflicts
SRT_PORT = 18890
TARGET = "127.0.0.1"


def parse_srt_word(data):
    """Decode the CDT (Control/Data Type) word"""
    if len(data) < 4:
        return None
    v = struct.unpack('>I', data[:4])[0]
    is_control = (v & 0x80000000) != 0
    if is_control:
        subtype = (v >> 16) & 0x7FFF
        lower16 = v & 0xFFFF
        return f"CONTROL subtype={subtype} (0x{v:08x}, lower16=0x{lower16:04x})"
    else:
        # Data packet bits
        boundary = (v >> 29) & 0x3
        in_order = (v >> 28) & 0x1
        return f"DATA bnd={boundary} ord={in_order} (0x{v:08x})"


def sniff_udp(port, duration=8):
    """Simple UDP sniffer using AF_PACKET"""
    try:
        from scapy.all import sniff, IP, UDP
        packets = []
        capture_done = threading.Event()

        def process(pkt):
            if UDP in pkt and IP in pkt:
                udp = pkt[UDP]
                if udp.dport == port or udp.sport == port:
                    payload = bytes(udp.payload)
                    direction = "->" if udp.dport == port else "<-"
                    print(f"\n[CAP] {pkt[IP].src} {direction} {pkt[IP].dst}:{udp.dport if direction=='->' else udp.sport}")
                    print(f"  Word0: {parse_srt_word(payload)}")
                    if len(payload) >= 8:
                        w1 = struct.unpack('>I', payload[4:8])[0]
                        print(f"  Word1 (0x{w1:08x}): {'DstSocketID' if (payload[0] & 0x80) else 'TSBPD'}")
                    if len(payload) >= 12:
                        w2 = struct.unpack('>I', payload[8:12])[0]
                        cdt = struct.unpack('>I', payload[:4])[0]
                        if (cdt & 0x80000000) and ((cdt >> 16) & 0x7FFF) == 0:
                            print(f"  Word2 (0x{w2:08x}): Version")
                    if len(payload) >= 16:
                        w3 = struct.unpack('>I', payload[12:16])[0]
                        print(f"  Word3 (0x{w3:08x}): MaxPayload={w3}")
                    if len(payload) >= 20:
                        w4 = struct.unpack('>I', payload[16:20])[0]
                        print(f"  Word4 (0x{w4:08x}): ExtFlags")
                    if len(payload) >= 24:
                        ext_type = struct.unpack('>H', payload[20:22])[0]
                        ext_len = struct.unpack('>H', payload[22:24])[0]
                        print(f"  Word5: ext_type=0x{ext_type:04x} ext_len={ext_len}")
                    # Full hex dump of first 60 bytes
                    hexdump = ' '.join(f'{b:02x}' for b in payload[:60])
                    print(f"  HEX: {hexdump}")
                    if len(payload) > 60:
                        print(f"  ... ({len(payload)} bytes total)")
                    packets.append(pkt)

        def stop_capture():
            time.sleep(duration)
            from scapy.all import conf
            capture_done.set()

        # Start capture in background thread
        cap_thread = threading.Thread(
            target=lambda: sniff(
                filter=f"udp port {SRT_PORT}",
                prn=process,
                stop_filter=lambda p: capture_done.is_set(),
                timeout=duration + 2,
                iface="lo"
            ),
            daemon=True
        )
        cap_thread.start()
        time.sleep(1)

        print("Starting ffmpeg SRT caller...")
        client = subprocess.Popen(
            ["ffmpeg", "-f", "lavfi", "-i", "testsrc=duration=8:size=128x96:rate=10",
             "-c:v", "libx264", "-tune", "zerolatency", "-preset", "ultrafast",
             "-f", "mpegts", f"srt://{TARGET}:{SRT_PORT}?mode=caller&latency=200"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )

        try:
            out, _ = client.communicate(timeout=15)
            print("FFmpeg output (last 500 chars):")
            print(out.decode()[-500:])
        except subprocess.TimeoutExpired:
            client.kill()

        cap_thread.join(timeout=5)

    except ImportError:
        print("ERROR: scapy not available")
        sys.exit(1)


def main():
    print("=" * 80)
    print("SRT Wire Format Analysis - Real SRT handshake capture")
    print("=" * 80)
    print(f"Run MediaMTX SRT listener on port {SRT_PORT} first!")
    print(f"Then this captures ffmpeg caller handshake")
    print()

    # Check if mediamtx is running with our port
    # The mediamtx.yml uses 8890, so we need a custom config
    print("NOTE: This script expects MediaMTX on port", SRT_PORT)
    print("      If not configured, it will still capture ffmpeg attempts")
    print()

    sniff_udp(SRT_PORT)


if __name__ == '__main__':
    main()