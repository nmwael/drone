#!/bin/bash
set -euo pipefail

echo "=== Generating Bifrost config ==="

CONFIG_DIR="/workspace/.config/bifrost"
CONFIG_FILE="$CONFIG_DIR/config.json"
STACK_FILE="/workspace/.devcontainer/stack.json"

mkdir -p "$CONFIG_DIR"

# Read stack.json for model configuration
if [ ! -f "$STACK_FILE" ]; then
    echo "Warning: $STACK_FILE not found, using default config"
    STACK_FILE="/dev/null"
fi

# Generate bifrost config with config_store enabled
cat > "$CONFIG_FILE" << 'EOF'
{
  "port": 8082,
  "host": "0.0.0.0",
  "log_level": "info",
  "config_store": {
    "enabled": true,
    "type": "sqlite",
    "config": {
      "path": "./config.db"
    }
  },
  "client": {
    "enforce_auth_on_inference": false
  },
  "providers": []
}
EOF

# If stack.json exists, populate providers from it
if [ -f "/workspace/.devcontainer/stack.json" ]; then
    echo "Populating providers from stack.json..."
    # This would be populated by jq from stack.json
    # For now, leaving empty - will be populated at runtime
fi

echo "Bifrost config written to $CONFIG_FILE"