#!/usr/bin/env python3
"""
Parse TS output from STANAG S3 Simulator serial capture.
Extracts KLV metadata from MPEG-TS packets.
"""

import re
import sys
import struct
from typing import List, Dict, Any

# MISB ST 0601 KLV Keys
KLV_KEYS = {
    0x0001: "Position (Lat/Lon/Height)",
    0x0002: "Velocity (NED)",
    0x0003: "Platform Heading",
    0x0102: "Barometric Altitude",
    0x0103: "Platform Temperature",
    0x0104: "Magnetic Heading",
    0xFFFF: "Timestamp",
}

def parse_ts_output(log_file: str) -> List[Dict]:
    """Parse TS output log and extract KLV packets."""
    
    # Pattern to match TS_OUT hex dumps
    ts_pattern = re.compile(r'TS_OUT:\s+([0-9A-F\s\.]+)')
    
    # Pattern to match KLV log lines
    klv_pattern = re.compile(r'Muxed KLV:\s+(\d+)\s+bytes on PID\s+0x([0-9A-F]+)')
    
    packets = []
    
    with open(log_file, 'r') as f:
        content = f.read()
    
    # Find all TS_OUT hex dumps
    ts_matches = ts_pattern.findall(content)
    
    for i, hex_str in enumerate(ts_matches):
        # Clean up hex string (remove spaces, dots, and newlines)
        hex_bytes = re.sub(r'[\s\.]', '', hex_str)
        
        try:
            # Convert to bytes
            ts_data = bytes.fromhex(hex_bytes)
            
            # Parse what we have (may be truncated)
            if len(ts_data) >= 4:
                # Parse as many complete 188-byte packets as possible
                for offset in range(0, len(ts_data), 188):
                    remaining = len(ts_data) - offset
                    if remaining >= 4:  # At least TS header
                        packet = ts_data[offset:offset+188] if remaining >= 188 else ts_data[offset:]
                        parsed = parse_ts_packet(packet, i)
                        if parsed:
                            packets.append(parsed)
        except ValueError as e:
            continue
    
    return packets

def parse_ts_packet(packet: bytes, frame_idx: int) -> Dict:
    """Parse a single 188-byte MPEG-TS packet."""
    if len(packet) < 4:
        return None
    
    # TS Header (4 bytes)
    sync = packet[0]
    if sync != 0x47:
        return None
    
    pid = ((packet[1] & 0x1F) << 8) | packet[2]
    adaptation_field = (packet[3] & 0x30) >> 4
    continuity_counter = packet[3] & 0x0F
    payload_unit_start = bool(packet[1] & 0x40)
    
    # Find payload start
    offset = 4
    if adaptation_field == 2 or adaptation_field == 3:
        # Adaptation field present
        if len(packet) > offset:
            af_len = packet[offset]
            offset += 1 + af_len
    
    # Extract payload
    payload = packet[offset:]
    
    result = {
        'frame': frame_idx,
        'sync': sync,
        'pid': pid,
        'pid_hex': f'0x{pid:04X}',
        'payload_unit_start': payload_unit_start,
        'adaptation_field': adaptation_field,
        'continuity_counter': continuity_counter,
        'payload_len': len(payload),
        'payload_hex': payload.hex()[:100]  # First 100 chars
    }
    
    # Parse KLV data if PID is 0x0101
    if pid == 0x0101 and len(payload) > 4:
        klv_data = parse_klv(payload)
        result['klv'] = klv_data
    
    return result

def parse_klv(data: bytes) -> List[Dict]:
    """Parse KLV data from payload."""
    klv_elements = []
    offset = 0
    
    while offset + 4 <= len(data):
        # KLV: 2-byte key, 2-byte length, value
        key = (data[offset] << 8) | data[offset + 1]
        length = (data[offset + 2] << 8) | data[offset + 3]
        offset += 4
        
        if offset + length > len(data):
            break
        
        value = data[offset:offset + length]
        offset += length
        
        # Parse value based on key
        klv_elements.append({
            'key': key,
            'key_hex': f'0x{key:04X}',
            'key_name': KLV_KEYS.get(key, 'Unknown'),
            'length': length,
            'value_hex': value.hex(),
            'value_parsed': parse_klv_value(key, value)
        })
    
    return klv_elements

def parse_klv_value(key: int, value: bytes) -> Any:
    """Parse KLV value based on key type."""
    if key == 0x0001 and len(value) == 12:  # Position
        lat = struct.unpack('>i', value[0:4])[0] / 1e7
        lon = struct.unpack('>i', value[4:8])[0] / 1e7
        height = struct.unpack('>i', value[8:12])[0] / 1000.0  # mm to m
        return {'lat': lat, 'lon': lon, 'height_m': height}
    
    elif key == 0x0002 and len(value) == 12:  # Velocity NED
        vn = struct.unpack('>i', value[0:4])[0] / 1000.0  # mm/s to m/s
        ve = struct.unpack('>i', value[4:8])[0] / 1000.0
        vd = struct.unpack('>i', value[8:12])[0] / 1000.0
        return {'vn_m_s': vn, 've_m_s': ve, 'vd_m_s': vd}
    
    elif key == 0x0003 and len(value) == 4:  # Heading
        heading = struct.unpack('>i', value)[0] / 1e5  # deg5 to deg
        return {'heading_deg': heading}
    
    elif key == 0x0102 and len(value) == 4:  # Baro Altitude
        alt = struct.unpack('>i', value)[0] / 1000.0  # mm to m
        return {'altitude_m': alt}
    
    elif key == 0x0103 and len(value) == 2:  # Temperature
        temp = struct.unpack('>h', value)[0] / 10.0  # temp*10 to degC
        return {'temp_c': temp}
    
    elif key == 0x0104 and len(value) == 4:  # Magnetic Heading
        heading = struct.unpack('>i', value)[0] / 1e5  # deg5 to deg
        return {'heading_deg': heading}
    
    elif key == 0xFFFF and len(value) == 4:  # Timestamp
        ts_us = struct.unpack('>I', value)[0]
        return {'timestamp_us': ts_us, 'timestamp_sec': ts_us / 1e6}
    
    return value.hex()

def print_packets(packets: List[Dict]):
    """Print parsed packets in readable format."""
    for p in packets:
        if not p:
            continue
            
        print(f"\n=== Frame {p['frame']} ===")
        print(f"  PID: {p['pid_hex']} ({'KLV' if p['pid'] == 0x0101 else 'Video'})")
        print(f"  CC: {p['continuity_counter']}, PUSI: {p['payload_unit_start']}, AF: {p['adaptation_field']}")
        print(f"  Payload: {p['payload_len']} bytes")
        
        if 'klv' in p and p['klv']:
            print("  KLV Elements:")
            for klv in p['klv']:
                print(f"    Key: {klv['key_hex']} ({klv['key_name']})")
                print(f"      Length: {klv['length']} bytes")
                print(f"      Value: {klv['value_parsed']}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 parse_ts_output.py <log_file>")
        sys.exit(1)
    
    log_file = sys.argv[1]
    print(f"Parsing {log_file}...")
    
    packets = parse_ts_output(log_file)
    
    if not packets:
        print("No packets found. Make sure log contains 'TS_OUT:' lines.")
        return
    
    print(f"\nFound {len(packets)} TS packets")
    
    # Filter KLV packets
    klv_packets = [p for p in packets if p['pid'] == 0x0101]
    print(f"KLV packets: {len(klv_packets)}")
    
    # Print first few packets
    print_packets(packets[:10])
    
    # Summary
    if klv_packets:
        print("\n=== KLV Summary ===")
        for p in klv_packets[:3]:
            if 'klv' in p:
                for klv in p['klv']:
                    print(f"  {klv['key_name']}: {klv['value_parsed']}")

if __name__ == '__main__':
    main()