#!/bin/bash
set -euo pipefail

echo "=== Post-create setup for drone sensor suite ==="

# Create required directories
mkdir -p /workspace/models
mkdir -p /workspace/.cache/llm-lab
mkdir -p /workspace/.config/bifrost
mkdir -p /workspace/.opencode

# Generate opencode password if not exists
if [ ! -f /workspace/.opencode-password ]; then
    openssl rand -base64 32 > /workspace/.opencode-password
    echo "Generated opencode password"
fi

# Fetch model weights if in local mode
if [ "${CLOUD_MODE:-false}" = "false" ]; then
    echo "=== Fetching local model weights ==="
    bash /workspace/.devcontainer/scripts/fetch-models.sh
else
    echo "=== Cloud mode enabled - skipping local model fetch ==="
    echo "Using NVIDIA NIM cloud models via: ${NVIDIA_NIM_ENDPOINT}"
fi

# Generate opencode config from template
if [ -f /workspace/.devcontainer/opencode.json ]; then
    cp /workspace/.devcontainer/opencode.json /workspace/.opencode/opencode.json
    echo "Copied opencode config"
fi

# Generate bifrost config if in local mode
if [ "${CLOUD_MODE:-false}" = "false" ]; then
    bash /workspace/.devcontainer/scripts/write-bifrost-config.sh
fi

# Set permissions
chmod +x /workspace/.devcontainer/scripts/*.sh

echo "=== Post-create complete ==="
echo "Run: bash scripts/use-profile.sh cloud (or local)"