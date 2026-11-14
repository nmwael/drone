#!/usr/bin/env python3
"""
STANAG S3 Sensor Simulator - Test Sender
Reads TS hex dumps + JSON telemetry from QEMU simulator stdin.
Sends TS packets via UDP/Unix socket to MediaMTX.
Writes JSON telemetry to sensor_data.json for dashboard polling.
"""
import argparse
import json
import re
import socket
import sys
import time
from typing import List, Optional

TS_PATTERN = re.compile(r"TS_OUT:\s+([0-9A-F\s\.]+)")
UDP_DEFAULT = "127.0.0.1:8888"
UNIX_SOCKET_DEFAULT = "/tmp/stanag.sock"
SENSOR_DATA_PATH = "/workspaces/drone/ground-station/mediamtx/sensor_data.json"


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


def parse_json_line(line: str):
    """Try to parse a line as JSON sensor telemetry."""
    line = line.strip()
    if not line.startswith("{"):
        return None
    try:
        data = json.loads(line)
        if any(k in data for k in ("lat", "lon", "alt", "heading", "gps", "imu", "baro", "aht20")):
            return data
    except json.JSONDecodeError:
        pass
    return None


def write_sensor_data(data):
    """Write sensor data to JSON file for HTTP dashboard polling."""
    try:
        with open(SENSOR_DATA_PATH, "w") as f:
            json.dump(data, f)
    except Exception as e:
        print(f"Error writing sensor data: {e}", file=sys.stderr)


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
    sensor_data_list = []

    if args.file:
        print(f"Reading TS packets from: {args.file}", file=sys.stderr)
        with open(args.file, "r") as f:
            content = f.read()
        matches = TS_PATTERN.findall(content)
        for hex_str in matches:
            packets.extend(parse_ts_hex(hex_str))
        # Also parse JSON lines from file
        for line in content.splitlines():
            data = parse_json_line(line)
            if data:
                write_sensor_data(data)
                sensor_data_list.append(data)

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
                print(f"Sending to Unix socket {args.unix_socket}", file=sys.stderr)
                total = send_packets(sock, packets, True, None, delay_ms, args.loop)
                print(f"Sent {total} packets total", file=sys.stderr)
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
                total = send_packets(sock, packets, False, target,
                                     delay_ms, args.loop)
                print(f"Sent {total} packets total", file=sys.stderr)
            finally:
                sock.close()
    else:
        print("Reading TS packets and sensor data from stdin...",
              file=sys.stderr)
        sys.stderr.flush()

        # Collect packets from stdin
        packets: List[bytes] = []
        sensor_data_list = []

        while True:
            line = sys.stdin.readline()
            if not line:
                break

            line = line.strip()
            if not line:
                continue

            # Try JSON first (telemetry from UART)
            data = parse_json_line(line)
            if data:
                write_sensor_data(data)
                sensor_data_list.append(data)
                continue

            # Try TS hex dump
            match = TS_PATTERN.match(line)
            if match:
                hex_str = match.group(1)
                hex_bytes = re.sub(r"[\s\.]", "", hex_str)
                try:
                    data_bytes = bytes.fromhex(hex_bytes)
                except ValueError:
                    continue
                for offset in range(0, len(data_bytes), 188):
                    packet = data_bytes[offset:offset + 188]
                    if len(packet) == 188 and packet[0] == 0x47:
                        packets.append(packet)
                continue

            # Unrecognized line - skip

        if not packets:
            print("No TS packets found!", file=sys.stderr)
            sys.exit(1)

        print(f"Found {len(packets)} TS packets",
              f"({len(sensor_data_list)} sensor frames)", file=sys.stderr)

        # Write raw TS bytes to stdout for piping to FFmpeg
        sys.stdout.buffer.write(b"".join(packets))
        sys.stdout.buffer.flush()

        # Create socket based on transport
        use_unix = args.use_unix_socket
        if use_unix:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                sock.connect(args.unix_socket)
                print(f"Sending to Unix socket {args.unix_socket} "
                      f"(delay: {delay_ms} ms, loop: {args.loop})",
                      file=sys.stderr)
                total = send_packets(sock, packets, True, None,
                                     delay_ms, args.loop)
                print(f"Sent {total} packets total", file=sys.stderr)
            finally:
                sock.close()
        else:
            try:
                host, port_str = args.udp.rsplit(":", 1)
                target = (host, int(port_str))
            except ValueError:
                print(f"Invalid --udp format: {args.udp} "
                      f"(expected host:port)",
                      file=sys.stderr)
                sys.exit(1)

            print(f"Sending to {target[0]}:{target[1]} "
                  f"(delay: {delay_ms} ms, loop: {args.loop})",
                  file=sys.stderr)

            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            try:
                total = send_packets(sock, packets, False, target,
                                     delay_ms, args.loop)
                print(f"Sent {total} packets total", file=sys.stderr)
            finally:
                sock.close()


if __name__ == "__main__":
    main()