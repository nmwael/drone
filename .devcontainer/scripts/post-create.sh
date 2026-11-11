#!/bin/bash
# Post-create setup for drone sensor suite devcontainer

# Don't exit on error - handle gracefully
set +e

echo "=== Post-create setup for drone sensor suite ==="

# Create required directories
mkdir -p /workspace/models
mkdir -p /workspace/.cache/llm-lab
mkdir -p /workspace/.config/bifrost
mkdir -p /workspace/.opencode

# Generate opencode password if not exists
if [ ! -f /workspace/.opencode-password ]; then
    if command -v openssl >/dev/null 2>&1; then
        openssl rand -base64 32 > /workspace/.opencode-password
        echo "Generated opencode password"
    else
        echo "WARNING: openssl not found, skipping password generation"
    fi
fi

# Fetch model weights if in local mode
if [ "${CLOUD_MODE:-true}" = "false" ]; then
    echo "=== Fetching local model weights ==="
    if [ -f /workspace/.devcontainer/scripts/fetch-models.sh ]; then
        bash /workspace/.devcontainer/scripts/fetch-models.sh || echo "WARNING: Model fetch failed"
    else
        echo "WARNING: fetch-models.sh not found"
    fi
else
    echo "=== Cloud mode enabled - skipping local model fetch ==="
    echo "Using NVIDIA NIM cloud models via: ${NVIDIA_NIM_ENDPOINT}"
fi

# Generate opencode config from template
if [ -f /workspace/.devcontainer/opencode.json ]; then
    cp /workspace/.devcontainer/opencode.json /workspace/.opencode/opencode.json 2>/dev/null && echo "Copied opencode config" || echo "WARNING: Failed to copy opencode config"
fi

# Generate bifrost config if in local mode
if [ "${CLOUD_MODE:-true}" = "false" ]; then
    if [ -f /workspace/.devcontainer/scripts/write-bifrost-config.sh ]; then
        bash /workspace/.devcontainer/scripts/write-bifrost-config.sh || echo "WARNING: bifrost config generation failed"
    fi
fi

# Set permissions on scripts
chmod +x /workspace/.devcontainer/scripts/*.sh 2>/dev/null || true

echo "=== Post-create complete ==="
echo "Run: bash scripts/use-profile.sh cloud (or local)"

exit 0