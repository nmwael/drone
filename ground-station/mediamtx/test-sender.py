#!/usr/bin/env python3
"""
TS Test Sender - Reads TS hex dumps from simulator serial output and sends
via UDP or Unix socket to MediaMTX for end-to-end testing.

Usage:
    # Option 1: Read from a log file captured from QEMU (UDP)
    python3 test-sender.py --file serial.log --udp 127.0.0.1:8888

    # Option 2: Pipe from QEMU output in real-time (UDP)
    idf.py -p /dev/null qemu monitor 2>&1 | python3 test-sender.py --udp 127.0.0.1:8888

    # Option 3: Send via Unix socket (QEMU/devcontainer, bypasses WSL2 UDP limits)
    idf.py -p /dev/null qemu monitor 2>&1 | python3 test-sender.py --unix-socket /tmp/stanag.sock

    # Option 4: Send a test pattern (loop TS packets from a captured file)
    python3 test-sender.py --file serial.log --udp 127.0.0.1:8888 --loop
"""

import argparse
import os
import re
import socket
import sys
import time
from typing import List, Optional


TS_PATTERN = re.compile(r"TS_OUT:\s+([0-9A-F\s\.]+)")
UDP_DEFAULT = "127.0.0.1:8888"
UNIX_SOCKET_DEFAULT = "/tmp/stanag.sock"


def parse_ts_hex(hex_str: str) -> List[bytes]:
    """Parse TS_OUT hex dump string into list of 188-byte TS packets."""
    hex_bytes = re.sub(r"[\s\.]", "", hex_str)
    try:
        data = bytes.fromhex(hex_bytes)
    except ValueError:
        return []

    packets = []
    for offset in range(0, len(data), 188):
        packet = data[offset : offset + 188]
        if len(packet) == 188 and packet[0] == 0x47:
            packets.append(packet)
    return packets


def send_packets(sock: socket.socket, packets: List[bytes],
                 use_unix: bool, target: tuple,
                 delay_ms: float = 33,
                 loop: bool = False) -> int:
    """Send TS packets via UDP or Unix socket. Returns total packets sent."""
    sent = 0
    iteration = 0
    while True:
        for pkt in packets:
            if use_unix:
                sock.send(pkt)
            else:
                sock.sendto(pkt, target)
            sent += 1
            if delay_ms > 0:
                time.sleep(delay_ms / 1000.0)
        iteration += 1
        if not loop:
            break
        print(f"  Loop iteration {iteration} complete, {sent} packets sent so far...",
              file=sys.stderr)
    return sent


def main():
    parser = argparse.ArgumentParser(
        description="Send TS packets from simulator log to MediaMTX via UDP or Unix socket"
    )
    parser.add_argument(
        "--file", "-f",
        help="Log file containing TS_OUT hex dumps (default: stdin)"
    )
    parser.add_argument(
        "--udp", "-u", default=UDP_DEFAULT,
        help=f"UDP target host:port (default: {UDP_DEFAULT})"
    )
    parser.add_argument(
        "--unix-socket", "-s", default=UNIX_SOCKET_DEFAULT,
        help=f"Unix socket path (default: {UNIX_SOCKET_DEFAULT})"
    )
    parser.add_argument(
        "--use-unix-socket", action="store_true",
        help="Send via Unix socket instead of UDP"
    )
    parser.add_argument(
        "--delay", "-d", type=float, default=33.0,
        help="Delay between packets in ms (default: 33 for 30 fps)"
    )
    parser.add_argument(
        "--loop", "-l", action="store_true",
        help="Loop the captured packets continuously"
    )
    parser.add_argument(
        "--no-delay", action="store_true",
        help="Send packets as fast as possible (no delay)"
    )
    args = parser.parse_args()

    delay_ms = 0.0 if args.no_delay else args.delay

    # Collect packets
    packets: List[bytes] = []

    if args.file:
        print(f"Reading TS packets from: {args.file}", file=sys.stderr)
        with open(args.file, "r") as f:
            content = f.read()
        matches = TS_PATTERN.findall(content)
        for hex_str in matches:
            packets.extend(parse_ts_hex(hex_str))
    else:
        print("Reading TS packets from stdin...", file=sys.stderr)
        content = sys.stdin.read()
        matches = TS_PATTERN.findall(content)
        for hex_str in matches:
            packets.extend(parse_ts_hex(hex_str))

    if not packets:
        print("No TS packets found!", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(packets)} TS packets", file=sys.stderr)

    # Write raw TS bytes to stdout for piping to FFmpeg
    sys.stdout.buffer.write(b"".join(packets))
    sys.stdout.buffer.flush()

    # Create socket based on transport
    use_unix = args.use_unix_socket
    if use_unix:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            sock.connect(args.unix_socket)
            print(f"Sending to Unix socket {args.unix_socket} (delay: {delay_ms} ms, "
                  f"loop: {args.loop})", file=sys.stderr)
            total = send_packets(sock, packets, True, None, delay_ms, args.loop)
            print(f"Sent {total} packets total", file=sys.stderr)
        except KeyboardInterrupt:
            print(f"\nInterrupted", file=sys.stderr)
        finally:
            sock.close()
    else:
        try:
            host, port_str = args.udp.rsplit(":", 1)
            target = (host, int(port_str))
        except ValueError:
            print(f"Invalid --udp format: {args.udp} (expected host:port)",
                  file=sys.stderr)
            sys.exit(1)

        print(f"Sending to {target[0]}:{target[1]} (delay: {delay_ms} ms, "
              f"loop: {args.loop})", file=sys.stderr)

        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            total = send_packets(sock, packets, False, target, delay_ms, args.loop)
            print(f"Sent {total} packets total", file=sys.stderr)
        except KeyboardInterrupt:
            print(f"\nInterrupted", file=sys.stderr)
        finally:
            sock.close()


if __name__ == "__main__":
    main()
