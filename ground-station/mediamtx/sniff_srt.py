#!/usr/bin/env python3
"""
SRT Traffic Sniffer - Capture ESP32 <-> 172.160.27.93:8890 SRT handshake

Usage:
    sudo python3 sniff_srt.py                       # Capture on all interfaces
    sudo python3 sniff_srt.py -i wlan0              # Capture on specific interface
    sudo python3 sniff_srt.py -w srt_capture.pcap   # Save to pcap for Wireshark
    sudo python3 sniff_srt.py --target 192.168.1.20 # Different target IP
"""

import argparse
import socket
import struct
import sys
from datetime import datetime

try:
    import scapy.all as scapy
    from scapy.layers.inet import IP, UDP
    SCAPY_AVAILABLE = True
except ImportError:
    SCAPY_AVAILABLE = False
    print("Warning: scapy not available. Install with: pip3 install scapy")
    print("Falling back to raw socket capture...\n")


SRT_PORT = 8890


def parse_srt_packet(data):
    """Parse and decode SRT packet"""
    if len(data) < 4:
        return None

    cdt = struct.unpack('>I', data[:4])[0]
    is_control = (cdt & 0x80000000) != 0

    result = {
        'cdt': cdt,
        'is_control': is_control,
        'length': len(data),
    }

    if is_control and len(data) >= 8:
        subtype = struct.unpack('>I', data[4:8])[0]
        result['subtype'] = subtype

        types = {
            0x00000000: 'HANDSHAKE',
            0x00000001: 'KEEPALIVE',
            0x00000002: 'ACK',
            0x00000003: 'NAK',
            0x00000005: 'SHUTDOWN',
            0x0000000F: 'EXTENDED',
        }
        result['type_name'] = types.get(subtype, f'UNKNOWN(0x{subtype:08x})')

        if subtype == 0x00000000:  # HANDSHAKE
            if len(data) >= 24:
                result['dst_sockid'] = struct.unpack('>I', data[8:12])[0]
                result['version'] = struct.unpack('>I', data[12:16])[0]
                result['max_payload'] = struct.unpack('>I', data[16:20])[0]
                result['ext_flags'] = struct.unpack('>I', data[20:24])[0]

                # Parse SID extension if present
                if result['ext_flags'] & 1 and len(data) >= 28:
                    pos = 24
                    while pos + 4 <= len(data):
                        ext_type = struct.unpack('>H', data[pos:pos+2])[0]
                        ext_len = struct.unpack('>H', data[pos+2:pos+4])[0]
                        if ext_type == 0x0005:  # SRT_CMD_SID
                            result['stream_id'] = data[pos+4:pos+4+ext_len].decode(
                                'utf-8', errors='replace')
                        pos += 4 + ext_len
                        while pos % 4:
                            pos += 1

        elif subtype == 0x00000002:  # ACK
            if len(data) >= 12:
                result['ack_seq'] = struct.unpack('>I', data[8:12])[0]

        elif subtype == 0x00000003:  # NAK
            if len(data) >= 10:
                result['nak_count'] = struct.unpack('>H', data[8:10])[0]

    else:  # DATA packet
        if len(data) >= 28:
            result['tsbpd'] = struct.unpack('>I', data[4:8])[0]
            result['timestamp'] = struct.unpack('>I', data[8:12])[0]
            result['dst_sockid'] = struct.unpack('>I', data[12:16])[0]
            result['seq'] = struct.unpack('>I', data[16:20])[0]
            result['msg'] = struct.unpack('>I', data[20:24])[0]
            result['payload_len'] = struct.unpack('>H', data[24:26])[0]
            result['flags'] = struct.unpack('>H', data[26:28])[0]

    return result


def format_packet(pkt_info, src_ip, dst_ip, direction, target_ip):
    """Format packet info for display"""
    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]

    if pkt_info['is_control']:
        lines = [
            f"[{timestamp}] {direction} {src_ip} -> {dst_ip} "
            f"SRT {pkt_info['type_name']} ({pkt_info['length']} bytes)",
        ]

        if 'dst_sockid' in pkt_info:
            lines.append(
                f"  DstSockID=0x{pkt_info['dst_sockid']:08x} "
                f"Version=0x{pkt_info.get('version', 0):08x} "
                f"MaxPayload={pkt_info.get('max_payload', 0)}")
        if 'stream_id' in pkt_info:
            lines.append(f"  StreamID: {pkt_info['stream_id']}")
        if 'ack_seq' in pkt_info:
            lines.append(f"  ACK Seq: {pkt_info['ack_seq']}")
        if 'nak_count' in pkt_info:
            lines.append(f"  NAK Count: {pkt_info['nak_count']}")

    else:
        lines = [
            f"[{timestamp}] {direction} {src_ip} -> {dst_ip} "
            f"SRT DATA ({pkt_info['length']} bytes)",
            f"  Seq={pkt_info.get('seq', 0)} Msg={pkt_info.get('msg', 0)} "
            f"PayloadLen={pkt_info.get('payload_len', 0)} "
            f"DstSockID=0x{pkt_info.get('dst_sockid', 0):08x}",
        ]

    return '\n'.join(lines)


def sniff_with_scapy(interface, bpf_filter, output_pcap, target_ip):
    """Sniff using scapy"""
    packets = []
    pkt_count = [0]

    def process_packet(pkt):
        if UDP in pkt and IP in pkt:
            udp = pkt[UDP]
            ip = pkt[IP]

            if udp.dport == SRT_PORT or udp.sport == SRT_PORT:
                direction = ">>" if ip.dst == target_ip else "<<"
                pkt_info = parse_srt_packet(bytes(udp.payload))
                if pkt_info:
                    pkt_count[0] += 1
                    print(format_packet(pkt_info, ip.src, ip.dst,
                                        direction, target_ip))
                packets.append(pkt)

    print(f"Starting SRT sniffer on {interface or 'all interfaces'}...")
    print(f"BPF filter: {bpf_filter or f'udp port {SRT_PORT}'}")
    print(f"Target IP: {target_ip}:{SRT_PORT}")
    print("-" * 80)

    try:
        scapy.sniff(
            iface=interface,
            filter=bpf_filter or f"udp port {SRT_PORT}",
            prn=process_packet,
            store=True if output_pcap else False
        )
    except KeyboardInterrupt:
        pass
    finally:
        if output_pcap and packets:
            scapy.wrpcap(output_pcap, packets)
            print(f"\nSaved {len(packets)} packets to {output_pcap}")


def sniff_raw_socket(interface, output_pcap, target_ip):
    """Sniff using raw socket (fallback without scapy)"""
    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
    if interface:
        try:
            sock.bind((interface, 0))
        except OSError as e:
            print(f"ERROR: Cannot bind to {interface}: {e}")
            sys.exit(1)

    packets = []
    pkt_count = 0

    print(f"Starting SRT sniffer on {interface or 'all interfaces'} (raw socket)...")
    print(f"Target IP: {target_ip}:{SRT_PORT}")
    print("-" * 80)

    try:
        while True:
            data, _ = sock.recvfrom(65535)

            if len(data) < 14:
                continue

            eth_type = struct.unpack('>H', data[12:14])[0]
            if eth_type != 0x0800:  # IPv4
                continue

            ip_header = data[14:34]
            if len(ip_header) < 20:
                continue

            version_ihl = ip_header[0]
            ihl = (version_ihl & 0x0F) * 4
            protocol = ip_header[9]
            src_ip = socket.inet_ntoa(ip_header[12:16])
            dst_ip = socket.inet_ntoa(ip_header[16:20])

            if protocol != 17:  # UDP
                continue

            udp_header = data[14 + ihl:14 + ihl + 8]
            if len(udp_header) < 8:
                continue

            src_port = struct.unpack('>H', udp_header[0:2])[0]
            dst_port = struct.unpack('>H', udp_header[2:4])[0]

            if src_port != SRT_PORT and dst_port != SRT_PORT:
                continue

            udp_payload = data[14 + ihl + 8:]
            direction = ">>" if dst_ip == target_ip else "<<"

            pkt_info = parse_srt_packet(udp_payload)
            if pkt_info:
                pkt_count += 1
                print(format_packet(pkt_info, src_ip, dst_ip,
                                    direction, target_ip))

            if output_pcap:
                packets.append(data)

    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
        if output_pcap and packets:
            print(f"\nSaved {len(packets)} raw packets to {output_pcap}")


def main():
    parser = argparse.ArgumentParser(
        description='SRT Traffic Sniffer for ESP32 STANAG stream')
    parser.add_argument('-i', '--interface',
                        help='Network interface (e.g., wlan0, eth0)')
    parser.add_argument('-f', '--filter',
                        help='BPF filter (default: udp port 8890)')
    parser.add_argument('-w', '--write',
                        help='Save capture to pcap file')
    parser.add_argument('--target', default='172.160.27.93',
                        help='Target IP (default: 172.160.27.93)')

    args = parser.parse_args()

    if SCAPY_AVAILABLE:
        sniff_with_scapy(args.interface, args.filter, args.write, args.target)
    else:
        sniff_raw_socket(args.interface, args.write, args.target)


if __name__ == '__main__':
    main()