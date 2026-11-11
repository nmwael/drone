#!/bin/bash
# Post-create setup for drone sensor suite devcontainer

# Don't exit on error - handle gracefully
set +e

echo "=== Post-create setup for drone sensor suite ==="

# Create required directories with proper permissions
# Use sudo if available, otherwise try without
create_dir() {
    local dir="$1"
    if [ ! -d "$dir" ]; then
        if mkdir -p "$dir" 2>/dev/null; then
            echo "Created directory: $dir"
        elif sudo mkdir -p "$dir" 2>/dev/null; then
            sudo chown -R $(whoami):$(whoami) "$dir" 2>/dev/null || true
            echo "Created directory (with sudo): $dir"
        else
            echo "WARNING: Cannot create directory: $dir"
        fi
    fi
}

create_dir "/workspace/models"
create_dir "/workspace/.cache/llm-lab"
create_dir "/workspace/.config/bifrost"
create_dir "/workspace/.opencode"

# Generate opencode password if not exists
if [ ! -f /workspace/.opencode-password ]; then
    if command -v openssl >/dev/null 2>&1; then
        if openssl rand -base64 32 > /workspace/.opencode-password 2>/dev/null; then
            echo "Generated opencode password"
        elif sudo openssl rand -base64 32 | sudo tee /workspace/.opencode-password >/dev/null 2>&1 && sudo chown $(whoami):$(whoami) /workspace/.opencode-password 2>/dev/null; then
            echo "Generated opencode password (with sudo)"
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
    if cp /workspace/.devcontainer/opencode.json /workspace/.opencode/opencode.json 2>/dev/null; then
        echo "Copied opencode config"
    elif sudo cp /workspace/.devcontainer/opencode.json /workspace/.opencode/opencode.json 2>/dev/null && sudo chown $(whoami):$(whoami) /workspace/.opencode/opencode.json 2>/dev/null; then
        echo "Copied opencode config (with sudo)"
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
chmod +x /workspace/.devcontainer/scripts/*.sh 2>/dev/null || sudo chmod +x /workspace/.devcontainer/scripts/*.sh 2>/dev/null || true

echo "=== Post-create complete ==="
echo "Run: bash scripts/use-profile.sh cloud (or local)"

exit 0