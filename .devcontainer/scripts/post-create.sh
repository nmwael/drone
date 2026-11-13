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

# Install ESP-IDF via EIM (ESP-IDF Installation Manager) if not already installed
if ! command -v eim >/dev/null 2>&1; then
  echo "=== Installing EIM (ESP-IDF Installation Manager) ==="
  sudo install -m 0755 -d /etc/apt/keyrings
  sudo curl -fsSL https://dl.espressif.com/dl/eim/eim.gpg -o /etc/apt/keyrings/eim.gpg
  sudo chmod 0644 /etc/apt/keyrings/eim.gpg
  sudo curl -fsSL https://dl.espressif.com/dl/eim/eim.sources -o /etc/apt/sources.list.d/espressif.sources
  sudo apt update
  sudo apt install -y eim-cli
fi

# Install ESP-IDF v5.5.6 via EIM
if ! eim list 2>/dev/null | grep -q "v5.5.6"; then
  echo "=== Installing ESP-IDF v5.5.6 via EIM ==="
  eim install -i v5.5.6 -t esp32s3 --cleanup true
fi

# Add EIM shell integration to bashrc
if ! grep -q "eim" "${HOME}/.bashrc" 2>/dev/null; then
  echo "" >> "${HOME}/.bashrc"
  echo "# ESP-IDF Installation Manager" >> "${HOME}/.bashrc"
  echo "export EIM_IDF_VERSION=v5.5.6" >> "${HOME}/.bashrc"
  echo "eval \"\$(eim completion bash)\" 2>/dev/null || true" >> "${HOME}/.bashrc"
fi

# Set up environment for current session
export EIM_IDF_VERSION=v5.5.6
if command -v eim >/dev/null 2>&1; then
  eim shell v5.5.6 2>/dev/null || true
fi

# Download MediaMTX for STANAG ground station testing
MEDIAMTX_DIR="${HOME}/ground-station/mediamtx"
MEDIAMTX_VER="v1.21.3"
if [ ! -x "${MEDIAMTX_DIR}/mediamtx" ]; then
  echo "=== Downloading MediaMTX ${MEDIAMTX_VER} ==="
  mkdir -p "${MEDIAMTX_DIR}"
  cd /tmp
  if wget -q "https://github.com/bluenviron/mediamtx/releases/download/${MEDIAMTX_VER}/mediamtx_${MEDIAMTX_VER}_linux_amd64.tar.gz" -O mediamtx.tar.gz 2>/dev/null; then
    tar xzf mediamtx.tar.gz -C "${MEDIAMTX_DIR}" mediamtx mediamtx.yml
    cp "${MEDIAMTX_DIR}/mediamtx.yml" "${MEDIAMTX_DIR}/mediamtx.yml.sample" 2>/dev/null || true
    chmod +x "${MEDIAMTX_DIR}/mediamtx"
    echo "MediaMTX downloaded to ${MEDIAMTX_DIR}"
  else
    echo "WARNING: Failed to download MediaMTX"
  fi
  rm -f /tmp/mediamtx.tar.gz
else
  echo "MediaMTX already present at ${MEDIAMTX_DIR}"
fi

# Configure MediaMTX for Unix socket ingestion (QEMU/devcontainer testing).
# This bypasses WSL2 UDP loopback limitations. Real hardware uses UDP via the
# committed mediamtx.yml and is unaffected by this devcontainer override.
echo "=== Configuring MediaMTX for Unix socket (QEMU testing) ==="
cat > "${MEDIAMTX_DIR}/mediamtx.yml" << 'MEDIAMTX_EOF'
###############################################
# MediaMTX Configuration for STANAG 4609 Testing
# Compatible with MediaMTX v1.21.3
# Devcontainer: Unix socket ingestion (bypasses WSL2 UDP limitations)
###############################################

# Global settings
logLevel: info
logDestinations: [stdout]
logStructured: false
logFile: ""

# API
api: yes
apiAddress: :9997

# Metrics
metrics: yes
metricsAddress: :9998

# RTSP
rtsp: yes
rtspAddress: :8554

# WebRTC
webrtc: yes
webrtcAddress: :8889
webrtcEncryption: false
webrtcAllowOrigins: ["*"]
webrtcLocalUDPAddress: :8189
webrtcIPsFromInterfaces: true

# Paths - Unix socket for QEMU/devcontainer testing
paths:
  stanag:
    source: unix+mpegts:///tmp/stanag.sock
    sourceOnDemand: no

  telemetry:
    source: unix+mpegts:///tmp/telemetry.sock
    sourceOnDemand: no

# HLS disabled
hls: no
MEDIAMTX_EOF

echo "MediaMTX configured for Unix socket at /tmp/stanag.sock"

# Set up TokenEconomizer for opencode (cost control for coding agents)
# See: https://github.com/microsoft/TokenEconomizer
if ! command -v python3 >/dev/null 2>&1; then
    echo "WARNING: python3 not found, skipping TokenEconomizer setup"
else
    echo "=== Setting up TokenEconomizer for opencode ==="
    TE_DIR="${HOME}/.tokeneconomizer"
    TE_REPO="${TE_DIR}/tokeneconomizer"
    if [ ! -d "${TE_REPO}" ]; then
        echo "=== Cloning TokenEconomizer ==="
        if git clone --depth 1 https://github.com/microsoft/TokenEconomizer.git "${TE_REPO}" 2>&1; then
            echo "Cloned TokenEconomizer to ${TE_REPO}"
        else
            echo "WARNING: Failed to clone TokenEconomizer, skipping setup"
            TE_REPO=""
        fi
    else
        echo "TokenEconomizer already present at ${TE_REPO}"
        git -C "${TE_REPO}" pull --depth 1 2>/dev/null || true
    fi

    if [ -n "${TE_REPO}" ]; then
        # Install TokenEconomizer Python package to get the `te` CLI
        if pip3 install --user "${TE_REPO}" 2>/dev/null; then
            echo "Installed TokenEconomizer Python package"
        else
            echo "WARNING: pip install failed, te CLI may not be available"
        fi

        # Symlink opencode plugin
        mkdir -p "${HOME}/.config/opencode/plugins" 2>/dev/null
        if ln -sf "${TE_REPO}/plugin/opencode/tokeneconomizer.ts" "${HOME}/.config/opencode/plugins/tokeneconomizer.ts" 2>/dev/null; then
            echo "Linked opencode plugin to ~/.config/opencode/plugins/"
        else
            echo "WARNING: Failed to symlink opencode plugin"
        fi

        # Find the te CLI
        TE_BIN=""
        if [ -x "${TE_DIR}/bin/te" ]; then
            TE_BIN="${TE_DIR}/bin/te"
        elif [ -x "${HOME}/.local/bin/te" ]; then
            TE_BIN="${HOME}/.local/bin/te"
        elif command -v te >/dev/null 2>&1; then
            TE_BIN="$(command -v te)"
        fi

        if [ -n "${TE_BIN}" ]; then
            TE_BIN_DIR="$(dirname "${TE_BIN}")"
            export PATH="${TE_BIN_DIR}:${PATH}"

            # Add to .bashrc for future sessions
            if ! grep -q "tokeneconomizer" "${HOME}/.bashrc" 2>/dev/null; then
                echo "" >> "${HOME}/.bashrc"
                echo "# TokenEconomizer" >> "${HOME}/.bashrc"
                echo "export PATH=\"${TE_BIN_DIR}:\${PATH}\"" >> "${HOME}/.bashrc"
            fi

            # Enable micro-skills for this repository (helps remember ESP-IDF
            # build steps, partition table config, PSRAM requirements, etc.)
            "${TE_BIN}" --host opencode config set microskills=on 2>/dev/null || \
                echo "WARNING: Could not set microskills=on"
            "${TE_BIN}" --host opencode status 2>/dev/null | head -5 || \
                echo "WARNING: Could not run te status"
        else
            echo "WARNING: te CLI not found after installation"
        fi
    fi
fi

echo "=== Post-create complete ==="
echo "Run: bash scripts/use-profile.sh cloud (or local)"

exit 0