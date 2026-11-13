#!/bin/bash
# Test Orchestrator - Start MediaMTX + test sender for STANAG simulator testing
#
# This script:
#   1. Starts MediaMTX in the background
#   2. Waits for it to be ready
#   3. Runs the test sender (from a captured log or stdin)
#   4. Verifies MediaMTX received the stream
#   5. Cleans up
#
# Usage:
#   ./test-orchestrator.sh --file serial.log          # Test from captured log (UDP)
#   ./test-orchestrator.sh --stdin                     # Pipe from QEMU (UDP)
#   ./test-orchestrator.sh --stdin --use-unix-socket   # Pipe from QEMU via Unix socket
#   ./test-orchestrator.sh --help                      # Show help

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MEDIAMTX_BIN="${SCRIPT_DIR}/mediamtx"
MEDIAMTX_CFG="${SCRIPT_DIR}/mediamtx.yml"
SENDER="${SCRIPT_DIR}/test-sender.py"
PIDFILE="${SCRIPT_DIR}/mediamtx.pid"

MEDIAMTX_PORT=8888
WEBRTC_PORT=8889
API_PORT=9997

usage() {
    echo "Usage: $0 {--file <logfile> | --stdin} [options]"
    echo ""
    echo "Options:"
    echo "  --file <logfile>   Read TS packets from simulator serial log"
    echo "  --stdin            Read TS packets from stdin (pipe from QEMU)"
    echo "  --delay <ms>       Delay between packets in ms (default: 33)"
    echo "  --no-delay         Send packets as fast as possible"
    echo "  --loop             Loop captured packets continuously"
    echo "  --media-only       Only start MediaMTX, don't run test sender"
    echo "  --stop             Stop MediaMTX and exit"
    echo "  --status           Check MediaMTX status and exit"
    echo ""
    echo "Examples:"
    echo "  $0 --file serial.log"
    echo "  idf.py -p /dev/null qemu monitor 2>&1 | $0 --stdin"
    echo "  $0 --media-only  # Just start MediaMTX for manual testing"
}

# Parse arguments
MODE=""
INPUT_FILE=""
DELAY=33
NO_DELAY=0
LOOP=0
USE_UNIX_SOCKET=0
MEDIA_ONLY=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --file) MODE="file"; INPUT_FILE="$2"; shift 2 ;;
        --stdin) MODE="stdin" ;;
        --use-unix-socket) USE_UNIX_SOCKET=1 ;;
        --delay) DELAY="$2"; shift 2 ;;
        --no-delay) NO_DELAY=1; DELAY=0 ;;
        --loop) LOOP=1 ;;
        --media-only) MEDIA_ONLY=1 ;;
        --stop)
            if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
                echo "Stopping MediaMTX (PID: $(cat ${PIDFILE}))..."
                kill "$(cat "${PIDFILE}")" 2>/dev/null || true
                rm -f "${PIDFILE}"
                echo "MediaMTX stopped"
            else
                echo "MediaMTX is not running"
            fi
            exit 0
            ;;
        --status)
            if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
                echo "MediaMTX is running (PID: $(cat ${PIDFILE}))"
                curl -sf "http://localhost:${API_PORT}/v3/paths/list" 2>/dev/null | python3 -m json.tool 2>/dev/null || \
                curl -sf "http://localhost:${API_PORT}/v3/paths/list" 2>/dev/null || true
            else
                echo "MediaMTX is not running"
            fi
            exit 0
            ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Unknown option: $1"; usage; exit 1 ;;
    esac
done

if [ "${MODE}" = "" ] && [ "${MEDIA_ONLY}" -eq 0 ]; then
    usage
    exit 1
fi

# Start MediaMTX
echo "=== Starting MediaMTX ==="
if [ -f "${PIDFILE}" ] && kill -0 "$(cat "${PIDFILE}")" 2>/dev/null; then
    echo "MediaMTX already running (PID: $(cat ${PIDFILE}))"
else
    cd "${SCRIPT_DIR}"
    nohup ./mediamtx "${MEDIAMTX_CFG}" > "${SCRIPT_DIR}/mediamtx-test.log" 2>&1 &
    echo $! > "${PIDFILE}"
    echo "MediaMTX started (PID: $(cat ${PIDFILE}))"
fi

# Wait for MediaMTX API
echo "Waiting for MediaMTX API (port ${API_PORT})..."
for i in $(seq 1 30); do
    if curl -sf "http://localhost:${API_PORT}/v3/paths/list" > /dev/null 2>&1; then
        echo "MediaMTX is ready"
        break
    fi
    if [ $i -eq 30 ]; then
        echo "WARNING: MediaMTX did not start in time"
        cat "${SCRIPT_DIR}/mediamtx-test.log" 2>/dev/null || true
        exit 1
    fi
    sleep 0.5
done

# Show current paths
echo ""
echo "=== MediaMTX Paths ==="
curl -sf "http://localhost:${API_PORT}/v3/paths/list" 2>/dev/null | python3 -m json.tool 2>/dev/null || \
curl -sf "http://localhost:${API_PORT}/v3/paths/list" 2>/dev/null || true

if [ "${MEDIA_ONLY}" -eq 1 ]; then
    echo ""
    echo "MediaMTX is running. Test endpoints:"
    if [ "${USE_UNIX_SOCKET}" -eq 1 ]; then
        echo "  Unix socket: /tmp/stanag.sock"
    else
        echo "  UDP ingest:    127.0.0.1:${MEDIAMTX_PORT}"
    fi
    echo ""
    echo "To stop: $0 --stop"
    exit 0
fi

# Run test sender
echo ""
echo "=== Running Test Sender ==="
SENDER_ARGS=""
if [ "${USE_UNIX_SOCKET}" -eq 1 ]; then
    SENDER_ARGS="--use-unix-socket --unix-socket /tmp/stanag.sock"
else
    SENDER_ARGS="--udp 127.0.0.1:${MEDIAMTX_PORT}"
fi
SENDER_ARGS="${SENDER_ARGS} --delay ${DELAY}"
if [ "${NO_DELAY}" -eq 1 ]; then
    SENDER_ARGS="--delay 0"
fi
if [ "${LOOP}" -eq 1 ]; then
    SENDER_ARGS="${SENDER_ARGS} --loop"
fi

if [ "${MODE}" = "file" ]; then
    SENDER_ARGS="${SENDER_ARGS} --file ${INPUT_FILE}"
fi

echo "Command: python3 ${SENDER} ${SENDER_ARGS}"
echo ""

if [ "${MODE}" = "stdin" ]; then
    echo "Piping stdin to test sender... (Ctrl+C to stop)"
    python3 "${SENDER}" ${SENDER_ARGS}
else
    python3 "${SENDER}" ${SENDER_ARGS}
fi

# Cleanup
echo ""
echo "=== Test Complete ==="
echo "MediaMTX is still running (PID: $(cat ${PIDFILE}))"
echo "To stop: $0 --stop"
echo "To check streams: curl http://localhost:${API_PORT}/v3/paths/list"
