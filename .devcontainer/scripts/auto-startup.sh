#!/bin/bash
set -euo pipefail

echo "=== Auto-startup: Drone Sensor Suite ==="

# Load environment
export CLOUD_MODE="${CLOUD_MODE:-false}"
export NVIDIA_API_KEY="${NVIDIA_API_KEY}"
export NVIDIA_NIM_ENDPOINT="${NVIDIA_NIM_ENDPOINT:-https://integrate.api.nvidia.com/v1}"

# Function to check if a port is in use
port_in_use() {
    ss -ltn | grep -q ":$1 "
}

# Function to wait for service
wait_for_service() {
    local url=$1
    local name=$2
    local max_attempts=30
    local attempt=1

    echo "Waiting for $name at $url..."
    while [ $attempt -le $max_attempts ]; do
        if curl -sf "$url" > /dev/null 2>&1; then
            echo "$name is ready"
            return 0
        fi
        sleep 2
        ((attempt++))
    done
    echo "ERROR: $name failed to start after $max_attempts attempts"
    return 1
}

# Cloud mode: skip llama-server and bifrost
if [ "${CLOUD_MODE:-false}" = "true" ]; then
    echo "=== CLOUD MODE: Starting opencode only ==="
    
    # Start opencode server
    if ! port_in_use 4096; then
        cd /workspace
        nohup opencode serve --port 4096 --config /workspace/.opencode/opencode.json > /workspace/.cache/opencode.log 2>&1 &
        echo "Started opencode on port 4096"
    else
        echo "opencode already running on port 4096"
    fi
    
    wait_for_service "http://localhost:4096/health" "opencode"
    echo "=== Cloud mode startup complete ==="
    exit 0
fi

# Local mode: start full stack
echo "=== LOCAL MODE: Starting full stack ==="

# Start llama-server orchestrator (port 8089)
if ! port_in_use 8089; then
    echo "Starting llama-server orchestrator on port 8089..."
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
        > /workspace/.cache/llama-orchestrator.log 2>&1 &
    echo "Started llama-server orchestrator"
else
    echo "llama-server orchestrator already running"
fi

# Start llama-server coder (port 8090)
if ! port_in_use 8090; then
    echo "Starting llama-server coder on port 8090..."
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
        > /workspace/.cache/llama-coder.log 2>&1 &
    echo "Started llama-server coder"
else
    echo "llama-server coder already running"
fi

# Wait for llama servers
wait_for_service "http://localhost:8089/health" "llama-server orchestrator"
wait_for_service "http://localhost:8090/health" "llama-server coder"

# Start bifrost gateway (port 8082)
if ! port_in_use 8082; then
    echo "Starting bifrost gateway on port 8082..."
    cd /workspace
    nohup bifrost \
        --config /workspace/.config/bifrost/config.json \
        --port 8082 \
        --host 0.0.0.0 \
        > /workspace/.cache/bifrost.log 2>&1 &
    echo "Started bifrost gateway"
else
    echo "bifrost gateway already running"
fi

wait_for_service "http://localhost:8082/health" "bifrost gateway"

# Reconcile bifrost config store
bash /workspace/.devcontainer/scripts/reconcile-bifrost.sh

# Start opencode server (port 4096)
if ! port_in_use 4096; then
    echo "Starting opencode server on port 4096..."
    cd /workspace
    nohup opencode serve \
        --port 4096 \
        --config /workspace/.opencode/opencode.json \
        --password-file /workspace/.opencode-password \
        > /workspace/.cache/opencode.log 2>&1 &
    echo "Started opencode server"
else
    echo "opencode server already running"
fi

wait_for_service "http://localhost:4096/health" "opencode"

echo "=== Local mode startup complete ==="
echo ""
echo "Services running:"
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