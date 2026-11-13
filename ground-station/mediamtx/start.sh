#!/bin/bash
# Start MediaMTX for STANAG 4609 testing with the ESP32-S3 simulator
#
# Ports:
#   8888  UDP   - STANAG MPEG-TS ingest from ESP32-S3
#   8889  TCP/UDP - WebRTC signaling + media (Oculus Quest browser)
#   8554  TCP   - RTSP (control uplink)
#   9997  HTTP  - MediaMTX API
#   9998  HTTP  - Prometheus metrics
#
# Usage:
#   ./start.sh                    # foreground
#   ./start.sh background         # background with nohup
#   ./start.sh stop               # stop background instance

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="${SCRIPT_DIR}/mediamtx.yml"
PIDFILE="${SCRIPT_DIR}/mediamtx.pid"
LOGFILE="${SCRIPT_DIR}/mediamtx.log"

case "${1:-foreground}" in
    foreground)
        echo "Starting MediaMTX (foreground) with config: ${CONFIG}"
        echo "  UDP ingest:    0.0.0.0:8888  (ESP32-S3 -> MediaMTX)"
        echo "  WebRTC:        0.0.0.0:8889  (Oculus Quest browser)"
        echo "  RTSP:          0.0.0.0:8554  (control uplink)"
        echo "  API:           0.0.0.0:9997  (MediaMTX REST API)"
        echo "  Metrics:       0.0.0.0:9998  (Prometheus)"
        echo ""
        exec ./mediamtx "${CONFIG}"
        ;;
    background)
        if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
            echo "MediaMTX already running (PID: $(cat ${PIDFILE}))"
            exit 0
        fi
        echo "Starting MediaMTX (background)..."
        nohup ./mediamtx "${CONFIG}" > "${LOGFILE}" 2>&1 &
        echo $! > "${PIDFILE}"
        echo "MediaMTX started (PID: $(cat ${PIDFILE}))"
        echo "Logs: ${LOGFILE}"
        echo ""
        echo "Wait a moment, then check:"
        echo "  curl http://localhost:9997/v3/paths/list"
        ;;
    stop)
        if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
            echo "Stopping MediaMTX (PID: $(cat ${PIDFILE}))..."
            kill "$(cat "${PIDFILE}")"
            rm -f "${PIDFILE}"
            echo "MediaMTX stopped"
        else
            echo "MediaMTX is not running"
        fi
        ;;
    status)
        if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
            echo "MediaMTX is running (PID: $(cat ${PIDFILE}))"
            echo "API: http://localhost:9997/v3/paths/list"
        else
            echo "MediaMTX is not running"
        fi
        ;;
    *)
        echo "Usage: $0 {foreground|background|stop|status}"
        exit 1
        ;;
esac
