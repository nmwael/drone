#!/bin/bash
# Post-create setup for drone sensor suite devcontainer

set +e

echo "=== Post-create setup for drone sensor suite ==="

# Use home directory for user-specific directories to avoid permission issues
MODELS_DIR="/workspace/models"
CACHE_DIR="${HOME}/.cache/llm-lab"
CONFIG_DIR="${HOME}/.config/bifrost"
OPENCODE_DIR="${HOME}/.opencode"

create_dir() {
    local dir="$1"
    if [ ! -d "$dir" ]; then
        if mkdir -p "$dir" 2>/dev/null; then
            echo "Created directory: $dir"
        else
            echo "WARNING: Cannot create directory: $dir"
        fi
    fi
}

create_dir "$MODELS_DIR"
create_dir "$CACHE_DIR"
create_dir "$CONFIG_DIR"
create_dir "$OPENCODE_DIR"

# Generate opencode password if not exists
if [ ! -f "${OPENCODE_DIR}/password" ]; then
    if command -v openssl >/dev/null 2>&1; then
        if openssl rand -base64 32 > "${OPENCODE_DIR}/password" 2>/dev/null; then
            echo "Generated opencode password"
        else
            echo "WARNING: Could not generate opencode password"
        fi
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
    if cp /workspace/.devcontainer/opencode.json "${HOME}/.opencode/opencode.json" 2>/dev/null; then
        echo "Copied opencode config"
    else
        echo "WARNING: Failed to copy opencode config"
    fi
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