#!/bin/bash
set -e

echo "Setting up ESP-IDF environment..."

# Install ESP-IDF if not present
if [ ! -d "/home/vscode/.espressif/v5.5.6/esp-idf" ]; then
    echo "Installing ESP-IDF v5.5.6..."
    mkdir -p /home/vscode/.espressif
    cd /home/vscode/.espressif
    git clone -b v5.5.6 --recursive https://github.com/espressif/esp-idf.git v5.5.6/esp-idf
    cd v5.5.6/esp-idf
    ./install.sh esp32s3 esp32c3
fi

# Create environment file for auto-sourcing
cat > /workspaces/drone/.devcontainer/esp-idf-env.sh << 'EOF'
# ESP-IDF Environment
export IDF_PATH=/home/vscode/.espressif/v5.5.6/esp-idf
export IDF_TOOLS_PATH=/home/vscode/.espressif
export PATH="/home/vscode/.espressif/python_env/idf5.5_py3.10_env/bin:$PATH"

# Source ESP-IDF export
if [ -f "/home/vscode/.espressif/v5.5.6/esp-idf/export.sh" ]; then
    source /home/vscode/.espressif/v5.5.6/esp-idf/export.sh > /dev/null 2>&1
fi

# Aliases for convenience
alias idf='idf.py'
alias menuconfig='idf.py menuconfig'
alias build='idf.py build'
alias flash='idf.py flash'
alias monitor='idf.py monitor'
alias flashmon='idf.py flash monitor'

# Navigate to project
cd /workspaces/drone/firmware/stanag-s3-simulator 2>/dev/null || cd /workspaces/drone
EOF

chmod +x /workspaces/drone/.devcontainer/esp-idf-env.sh

# Install clangd via pip for latest version (user-level install for vscode)
if [ ! -d "/home/vscode/.local" ]; then
    mkdir -p /home/vscode/.local
fi
chown -R vscode:vscode /home/vscode/.local 2>/dev/null || true
sudo -u vscode pip install --user clangd 2>/dev/null || pip install --user clangd 2>/dev/null || true

# Add clangd to PATH in environment file
cat >> /workspaces/drone/.devcontainer/esp-idf-env.sh << 'EOF'

# Add clangd (LSP) to PATH
export PATH="/home/vscode/.local/bin:$PATH"
EOF

echo "ESP-IDF setup complete!"
echo "Run 'source /workspaces/drone/.devcontainer/esp-idf-env.sh' or restart terminal"