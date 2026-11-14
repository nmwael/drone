#!/usr/bin/env python3
"""
STANAG S3 Sensor Simulator Data Reader
Reads UART telemetry JSON from QEMU simulator stdin and writes to data.json
for the dashboard to consume via HTTP.

Usage:
    idf.py -p /dev/null qemu monitor 2>&1 | python3 test-sensor-sim.py
"""

import sys
import json
import re
import os

SENSOR_DATA_PATH = "/workspaces/drone/ground-station/mediamtx/sensor_data.json"

def parse_sensor_line(line):
    """Try to parse a line as JSON sensor telemetry."""
    line = line.strip()
    if not line.startswith("{"):
        return None
    try:
        data = json.loads(line)
        # Validate it looks like our sensor data
        if "gps" in data or "imu" in data or "baro" in data or "aht20" in data:
            return data
    except json.JSONDecodeError:
        pass
    return None


def main():
    print("Sensor sim data reader started", file=sys.stderr)
    sys.stderr.flush()

    while True:
        line = sys.stdin.readline()
        if not line:
            break

        data = parse_sensor_line(line)
        if data:
            try:
                with open(SENSOR_DATA_PATH, "w") as f:
                    json.dump(data, f)
                f.flush()
                os.fsync(f.fileno())
            except Exception as e:
                print(f"Error writing sensor data: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()