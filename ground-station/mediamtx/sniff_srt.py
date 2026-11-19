#!/usr/bin/env python3
"""
SRT Traffic Sniffer - Run on host machine to capture ESP32 <-> 172.160.27.93 SRT handshake

Usage:
    python3 sniff_srt.py                    # Capture on all interfaces
    python3 sniff_srt.py -i wlan0           # Capture on specific interface
    python3 sniff_srt.py -f "host 172.160.27.93 and udp port 8890"  # BPF filter
    python3 sniff_srt.py -w srt_capture.pcap  # Save to pcap for Wireshark
"""

import argparse
import socket
import struct
import sys
import time
from datetime import datetime

try:
    import scapy.all as scapy
    from scapy.layers.inet import IP, UDP
    from scapy.layers.l2 import Ether
    SCAPY_AVAILABLE = True
except ImportError:
    SCAPY_AVAILABLE = False
    print("Warning: scapy not available. Install with: pip3 install scapy")
    print("Falling back to raw socket capture...")


SRT_PORT = 8890
TARGET_IP = "172.160.27.93"


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
        
        # Control packet types
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
                dst_sockid = struct.unpack('>I', data[8:12])[0]
                version = struct.unpack('>I', data[12:16])[0]
                max_payload = struct.unpack('>I', data[16:20])[0]
                ext_flags = struct.unpack('>I', data[20:24])[0]
                
                result['dst_sockid'] = dst_sockid
                result['version'] = version
                result['max_payload'] = max_payload
                result['ext_flags'] = ext_flags
                
                # Parse SID extension if present
                if ext_flags & 1 and len(data) >= 28:
                    pos = 24
                    while pos + 4 <= len(data):
                        ext_type = struct.unpack('>H', data[pos:pos+2])[0]
                        ext_len = struct.unpack('>H', data[pos+2:pos+4])[0]
                        if ext_type == 0x0005:  # SRT_CMD_SID
                            sid = data[pos+4:pos+4+ext_len].decode('utf-8', errors='replace')
                            result['stream_id'] = sid
                        pos += 4 + ext_len
                        while pos % 4:
                            pos += 1
        
        elif subtype == 0x00000002:  # ACK
            if len(data) >= 12:
                ack_seq = struct.unpack('>I', data[8:12])[0]
                result['ack_seq'] = ack_seq
                
        elif subtype == 0x00000003:  # NAK
            if len(data) >= 10:
                count = struct.unpack('>H', data[8:10])[0]
                result['nak_count'] = count
                
    else:  # DATA packet
        if len(data) >= 28:
            # DATA header: CDT(4) | TSBPD(4) | timestamp(4) | dst_sockid(4) | seq(4) | msg(4) | len(2) | flags(2)
            tsbpd = struct.unpack('>I', data[4:8])[0]
            timestamp = struct.unpack('>I', data[8:12])[0]
            dst_sockid = struct.unpack('>I', data[12:16])[0]
            seq = struct.unpack('>I', data[16:20])[0]
            msg = struct.unpack('>I', data[20:24])[0]
            payload_len = struct.unpack('>H', data[24:26])[0]
            flags = struct.unpack('>H', data[26:28])[0]
            
            result.update({
                'tsbpd': tsbpd,
                'timestamp': timestamp,
                'dst_sockid': dst_sockid,
                'seq': seq,
                'msg': msg,
                'payload_len': payload_len,
                'flags': flags,
            })
    
    return result


def format_packet(pkt_info, src_ip, dst_ip, direction):
    """Format packet info for display"""
    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
    
    if pkt_info['is_control']:
        lines = [
            f"[{timestamp}] {direction} {src_ip}:{SRT_PORT} -> {dst_ip}:{SRT_PORT} SRT {pkt_info['type_name']} ({pkt_info['length']} bytes)",
            f"  CDT=0x{pkt_info['cdt']:08x} Subtype=0x{pkt_info.get('subtype',0):08x}",
        ]
        
        if 'dst_sockid' in pkt_info:
            lines.append(f"  DstSockID=0x{pkt_info['dst_sockid']:08x} Version=0x{pkt_info.get('version',0):08x} MaxPayload={pkt_info.get('max_payload',0)}")
        if 'stream_id' in pkt_info:
            lines.append(f"  StreamID: {pkt_info['stream_id']}")
        if 'ack_seq' in pkt_info:
            lines.append(f"  ACK Seq: {pkt_info['ack_seq']}")
        if 'nak_count' in pkt_info:
            lines.append(f"  NAK Count: {pkt_info['nak_count']}")
            
    else:
        lines = [
            f"[{timestamp}] {direction} {src_ip}:{SRT_PORT} -> {dst_ip}:{SRT_PORT} SRT DATA ({pkt_info['length']} bytes)",
            f"  Seq={pkt_info.get('seq',0)} Msg={pkt_info.get('msg',0)} PayloadLen={pkt_info.get('payload_len',0)} DstSockID=0x{pkt_info.get('dst_sockid',0):08x}",
        ]
    
    return '\n'.join(lines)


def sniff_with_scapy(interface, bpf_filter, output_pcap):
    """Sniff using scapy"""
    packets = []
    
    def process_packet(pkt):
        if UDP in pkt and IP in pkt:
            udp = pkt[UDP]
            ip = pkt[IP]
            
            if udp.dport == SRT_PORT or udp.sport == SRT_PORT:
                direction = ">>" if ip.dst == TARGET_IP else "<<"
                pkt_info = parse_srt_packet(bytes(udp.payload))
                if pkt_info:
                    print(format_packet(pkt_info, ip.src, ip.dst, direction))
                packets.append(pkt)
    
    print(f"Starting SRT sniffer on {interface or 'all interfaces'}...")
    print(f"Filter: {bpf_filter or f'udp port {SRT_PORT}'}")
    print(f"Target: {TARGET_IP}:{SRT_PORT}")
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


def sniff_raw_socket(interface, output_pcap):
    """Sniff using raw socket (fallback without scapy)"""
    # Create raw socket
    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
    if interface:
        sock.bind((interface, 0))
    
    packets = []
    
    print(f"Starting SRT sniffer on {interface or 'all interfaces'} (raw socket)...")
    print(f"Target: {TARGET_IP}:{SRT_PORT}")
    print("-" * 80)
    
    try:
        while True:
            data, _ = sock.recvfrom(65535)
            
            # Parse Ethernet frame
            if len(data) < 14:
                continue
            
            eth_type = struct.unpack('>H', data[12:14])[0]
            if eth_type != 0x0800:  # IPv4
                continue
            
            ip_header = data[14:34]
            if len(ip_header) < 20:
                continue
            
            # Parse IP header
            version_ihl = ip_header[0]
            ihl = (version_ihl & 0x0F) * 4
            protocol = ip_header[9]
            src_ip = socket.inet_ntoa(ip_header[12:16])
            dst_ip = socket.inet_ntoa(ip_header[16:20])
            
            if protocol != 17:  # UDP
                continue
            
            udp_header = data[14+ihl:14+ihl+8]
            if len(udp_header) < 8:
                continue
            
            src_port = struct.unpack('>H', udp_header[0:2])[0]
            dst_port = struct.unpack('>H', udp_header[2:4])[0]
            
            if src_port != SRT_PORT and dst_port != SRT_PORT:
                continue
            
            udp_payload = data[14+ihl+8:]
            direction = ">>" if dst_ip == TARGET_IP else "<<"
            
            pkt_info = parse_srt_packet(udp_payload)
            if pkt_info:
                print(format_packet(pkt_info, src_ip, dst_ip, direction))
            
            if output_pcap:
                packets.append(data)
                
    except KeyboardInterrupt:
        pass
    finally:
        if output_pcap and packets:
            print(f"\nSaved {len(packets)} raw packets to {output_pcap}")


def main():
    parser = argparse.ArgumentParser(description='SRT Traffic Sniffer for ESP32 STANAG stream')
    parser.add_argument('-i', '--interface', help='Network interface (e.g., wlan0, eth0)')
    parser.add_argument('-f', '--filter', help='BPF filter (default: udp port 8890)')
    parser.add_argument('-w', '--write', help='Save capture to pcap file')
    parser.add_argument('--target', default=TARGET_IP, help=f'Target IP (default: {TARGET_IP})')
    
    args = parser.parse_args()
    
    global TARGET_IP
    TARGET_IP = args.target
    
    if SCAPY_AVAILABLE:
        sniff_with_scapy(args.interface, args.filter, args.write)
    else:
        sniff_raw_socket(args.interface, args.write)


if __name__ == '__main__':
    main()