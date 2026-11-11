#!/bin/bash
# Auto-startup script for drone sensor suite devcontainer
# Handles both cloud mode (NVIDIA NIM) and local mode (llama-server + bifrost)

set +e

echo "=== Auto-startup: Drone Sensor Suite ==="

# Load environment
export CLOUD_MODE="${CLOUD_MODE:-true}"
export NVIDIA_API_KEY="${NVIDIA_API_KEY}"
export NVIDIA_NIM_ENDPOINT="${NVIDIA_NIM_ENDPOINT:-https://integrate.api.nvidia.com/v1}"

# Use home directory for user-specific paths
CACHE_DIR="${HOME}/.cache/llm-lab"
OPENCODE_DIR="${HOME}/.opencode"
CONFIG_DIR="${HOME}/.config/bifrost"
MODELS_DIR="/workspace/models"

# Function to check if a port is in use
port_in_use() {
    if command -v ss >/dev/null 2>&1; then
        ss -ltn 2>/dev/null | grep -q ":$1 "
    elif command -v netstat >/dev/null 2>&1; then
        netstat -ltn 2>/dev/null | grep -q ":$1 "
    else
        timeout 1 bash -c "cat < /dev/null > /dev/tcp/127.0.0.1/$1" 2>/dev/null
    fi
}

# Function to wait for service with timeout
wait_for_service() {
    local url=$1
    local name=$2
    local max_attempts=30
    local attempt=1

    echo "Waiting for $name at $url..."
    while [ $attempt -le $max_attempts ]; do
        if curl -sf --max-time 2 "$url" > /dev/null 2>&1; then
            echo "$name is ready"
            return 0
        fi
        sleep 2
        ((attempt++))
    done
    echo "WARNING: $name failed to start after $max_attempts attempts (continuing anyway)"
    return 1
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

safe_log() {
    local logfile="$1"
    local message="$2"
    local dir=$(dirname "$logfile")
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir" 2>/dev/null || true
    fi
    echo "$message" >> "$logfile" 2>/dev/null || echo "$message"
}

# Cloud mode: start opencode only
if [ "${CLOUD_MODE:-true}" = "true" ]; then
    echo "=== CLOUD MODE: Starting opencode only ==="
    
    if ! command_exists opencode; then
        echo "WARNING: opencode not found in PATH, skipping startup"
        echo "You can start it manually later with: opencode serve --port 4096 --config ${HOME}/.opencode/opencode.json"
        exit 0
    fi
    
    if ! port_in_use 4096; then
        cd /workspace
        echo "Starting opencode on port 4096..."
        mkdir -p "${HOME}/.cache" 2>/dev/null || true
        nohup opencode serve --port 4096 --config "${HOME}/.opencode/opencode.json" > "${HOME}/.cache/opencode.log" 2>&1 &
        OPCODE_PID=$!
        echo "Started opencode (PID: $OPCODE_PID)"
    else
        echo "opencode already running on port 4096"
    fi
    
    wait_for_service "http://localhost:4096/health" "opencode"
    echo "=== Cloud mode startup complete ==="
    exit 0
fi

echo "=== LOCAL MODE: Starting full stack ==="

if ! command_exists llama-server; then
    echo "WARNING: llama-server not found, skipping local mode services"
    echo "Install llama-server to enable local mode services"
    exit 0
fi

# Start llama-server orchestrator (port 8089)
if ! port_in_use 8089; then
    echo "Starting llama-server orchestrator on port 8089..."
    if [ -f /workspace/models/gemma-4-12b-ud-iq3_m.gguf ]; then
        mkdir -p "${HOME}/.cache/llm-lab" 2>/dev/null || true
        nohup llama-server \
            --model /workspace/models/gemma-4-12b-ud-iq3_m.gguf \
            --port 8089 \
            --host 0.0.0.0 \
            --ctx-size 16384 \
            --batch-size 512 \
            --ubatch-size 512 \
            --threads $(nproc) \
            --gpu-layers -1 \
            --parallel 3 \
            --cache-type-k q8_0 \
            --cache-type-v q8_0 \
            --n-gpu-layers -1 \
            > "${HOME}/.cache/llm-lab/llama-orchestrator.log" 2>&1 &
        echo "Started llama-server orchestrator"
    else
        echo "WARNING: Model file not found, skipping llama-server orchestrator"
    fi
else
    echo "llama-server orchestrator already running"
fi

# Start llama-server coder (port 8090)
if ! port_in_use 8090; then
    echo "Starting llama-server coder on port 8090..."
    if [ -f /workspace/models/qwen2.5-coder-7b-q4_k_m.gguf ]; then
        mkdir -p "${HOME}/.cache/llm-lab" 2>/dev/null || true
        nohup llama-server \
            --model /workspace/models/qwen2.5-coder-7b-q4_k_m.gguf \
            --port 8090 \
            --host 0.0.0.0 \
            --ctx-size 16384 \
            --batch-size 512 \
            --ubatch-size 512 \
            --threads $(nproc) \
            --gpu-layers -1 \
            --parallel 1 \
            --cache-type-k q4_0 \
            --cache-type-v q8_0 \
            --n-gpu-layers -1 \
            > "${HOME}/.cache/llm-lab/llama-coder.log" 2>&1 &
        echo "Started llama-server coder"
    else
        echo "WARNING: Model file not found, skipping llama-server coder"
    fi
else
    echo "llama-server coder already running"
fi

wait_for_service "http://localhost:8089/health" "llama-server orchestrator" || true
wait_for_service "http://localhost:8090/health" "llama-server coder" || true

# Start bifrost gateway (port 8082)
if ! port_in_use 8082; then
    echo "Starting bifrost gateway on port 8082..."
    if command_exists bifrost && [ -f "${HOME}/.config/bifrost/config.json" ]; then
        mkdir -p "${HOME}/.cache/llm-lab" 2>/dev/null || true
        cd /workspace
        nohup bifrost \
            --config "${HOME}/.config/bifrost/config.json" \
            --port 8082 \
            --host 0.0.0.0 \
            > "${HOME}/.cache/llm-lab/bifrost.log" 2>&1 &
        echo "Started bifrost gateway"
    else
        echo "WARNING: bifrost not found or config missing, skipping"
    fi
else
    echo "bifrost gateway already running"
fi

wait_for_service "http://localhost:8082/health" "bifrost gateway" || true

if [ -f /workspace/.devcontainer/scripts/reconcile-bifrost.sh ]; then
    bash /workspace/.devcontainer/scripts/reconcile-bifrost.sh || echo "WARNING: bifrost reconciliation failed"
fi

if ! command_exists opencode; then
    echo "WARNING: opencode not found, skipping"
else
    if ! port_in_use 4096; then
        echo "Starting opencode server on port 4096..."
        cd /workspace
        nohup opencode serve \
            --port 4096 \
            --config "${HOME}/.opencode/opencode.json" \
            --password-file "${HOME}/.opencode/password" \
            > "${HOME}/.cache/opencode.log" 2>&1 &
        echo "Started opencode server"
    else
        echo "opencode server already running"
    fi
    
    wait_for_service "http://localhost:4096/health" "opencode" || true
fi

echo "=== Local mode startup complete ==="
echo ""
echo "Services running (check logs if any failed):"
echo "  - llama-server orchestrator: http://localhost:8089"
echo "  - llama-server coder: http://localhost:8090"
echo "  - bifrost gateway: http://localhost:8082"
echo "  - opencode: http://localhost:4096"
echo ""
echo "Health checks:"
echo "  curl http://localhost:8082/health"
echo "  curl http://localhost:8089/health"
echo "  curl http://localhost:8090/health"
echo "  curl http://localhost:4096/health"

exit 0