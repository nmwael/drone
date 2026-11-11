#!/bin/bash
# Generate Bifrost config with config_store enabled

set +e

echo "=== Generating Bifrost config ==="

CONFIG_DIR="${HOME}/.config/bifrost"
CONFIG_FILE="$CONFIG_DIR/config.json"
STACK_FILE="/workspace/.devcontainer/stack.json"

mkdir -p "$CONFIG_DIR"

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

if [ -f "/workspace/.devcontainer/stack.json" ]; then
    echo "Populating providers from stack.json..."
fi

echo "Bifrost config written to $CONFIG_FILE"

exit 0