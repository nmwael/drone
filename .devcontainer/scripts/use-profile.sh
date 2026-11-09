#!/bin/bash
set -euo pipefail

PROFILE="${1:-cloud}"
VALID_PROFILES=("cloud" "local")

# Validate profile
if [[ ! " ${VALID_PROFILES[@]} " =~ " ${PROFILE} " ]]; then
    echo "Error: Invalid profile '$PROFILE'. Valid profiles: ${VALID_PROFILES[*]}"
    exit 1
fi

echo "=== Switching to profile: $PROFILE ==="

# Update active profile marker
echo "$PROFILE" > /workspace/.devcontainer/active-profile

# Update llm-lab-models.json activeProfile
jq --arg profile "$PROFILE" '.activeProfile = $profile' /workspace/.devcontainer/llm-lab-models.json > /tmp/llm-lab-models.json.tmp
mv /tmp/llm-lab-models.json.tmp /workspace/.devcontainer/llm-lab-models.json

# Update llm-lab-roles.json activeProfile
jq --arg profile "$PROFILE" '.activeProfile = $profile' /workspace/.devcontainer/llm-lab-roles.json > /tmp/llm-lab-roles.json.tmp
mv /tmp/llm-lab-roles.json.tmp /workspace/.devcontainer/llm-lab-roles.json

# Copy profile-specific configs
if [ -f "/workspace/.devcontainer/profiles/combo-${PROFILE}.json" ]; then
    echo "Loading profile config: combo-${PROFILE}.json"
    # Profile-specific model list would be merged here
fi

if [ -f "/workspace/.devcontainer/profiles/roles-${PROFILE}.json" ]; then
    echo "Loading roles config: roles-${PROFILE}.json"
fi

# Update CLOUD_MODE environment
if [ "$PROFILE" = "cloud" ]; then
    export CLOUD_MODE=true
    echo "CLOUD_MODE=true" > /workspace/.env.cloud_mode
else
    export CLOUD_MODE=false
    echo "CLOUD_MODE=false" > /workspace/.env.cloud_mode
fi

# Regenerate opencode config
if [ -f /workspace/.devcontainer/opencode.json ]; then
    cp /workspace/.devcontainer/opencode.json /workspace/.opencode/opencode.json
    echo "Regenerated opencode config"
fi

# Regenerate bifrost config if local mode
if [ "$PROFILE" = "local" ]; then
    bash /workspace/.devcontainer/scripts/write-bifrost-config.sh
fi

# Restart services if running
if pgrep -f "opencode serve" > /dev/null; then
    echo "Restarting opencode..."
    pkill -f "opencode serve" || true
    sleep 2
    nohup opencode serve --port 4096 --config /workspace/.opencode/opencode.json --password-file /workspace/.opencode-password > /workspace/.cache/opencode.log 2>&1 &
fi

if [ "$PROFILE" = "local" ]; then
    if pgrep -f "bifrost" > /dev/null; then
        echo "Restarting bifrost..."
        pkill -f "bifrost" || true
        sleep 2
        nohup bifrost --config /workspace/.config/bifrost/config.json --port 8082 --host 0.0.0.0 > /workspace/.cache/bifrost.log 2>&1 &
        sleep 3
        bash /workspace/.devcontainer/scripts/reconcile-bifrost.sh
    fi
fi

echo "Profile switched to: $PROFILE"
echo ""
echo "To apply changes immediately in running container:"
echo "  source /workspace/.env.cloud_mode"
echo ""
echo "Or restart container:"
echo "  devcontainer up --workspace-folder ."