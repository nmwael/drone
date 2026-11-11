#!/bin/bash
# Switch between cloud and local profiles

set +e

PROFILE="${1:-cloud}"
VALID_PROFILES=("cloud" "local")

if [[ ! " ${VALID_PROFILES[@]} " =~ " ${PROFILE} " ]]; then
    echo "Error: Invalid profile '$PROFILE'. Valid profiles: ${VALID_PROFILES[*]}"
    exit 1
fi

echo "=== Switching to profile: $PROFILE ==="

echo "$PROFILE" > /workspace/.devcontainer/active-profile 2>/dev/null || true

if [ -f /workspace/.devcontainer/llm-lab-models.json ] && command -v jq >/dev/null 2>&1; then
    jq --arg profile "$PROFILE" '.activeProfile = $profile' /workspace/.devcontainer/llm-lab-models.json > /tmp/llm-lab-models.json.tmp 2>/dev/null && \
    mv /tmp/llm-lab-models.json.tmp /workspace/.devcontainer/llm-lab-models.json 2>/dev/null || echo "WARNING: Failed to update llm-lab-models.json"
fi

if [ -f /workspace/.devcontainer/llm-lab-roles.json ] && command -v jq >/dev/null 2>&1; then
    jq --arg profile "$PROFILE" '.activeProfile = $profile' /workspace/.devcontainer/llm-lab-roles.json > /tmp/llm-lab-roles.json.tmp 2>/dev/null && \
    mv /tmp/llm-lab-roles.json.tmp /workspace/.devcontainer/llm-lab-roles.json 2>/dev/null || echo "WARNING: Failed to update llm-lab-roles.json"
fi

if [ -f "/workspace/.devcontainer/profiles/combo-${PROFILE}.json" ]; then
    echo "Loading profile config: combo-${PROFILE}.json"
fi

if [ -f "/workspace/.devcontainer/profiles/roles-${PROFILE}.json" ]; then
    echo "Loading roles config: roles-${PROFILE}.json"
fi

if [ "$PROFILE" = "cloud" ]; then
    export CLOUD_MODE=true
    echo "CLOUD_MODE=true" > /workspace/.env.cloud_mode 2>/dev/null || true
else
    export CLOUD_MODE=false
    echo "CLOUD_MODE=false" > /workspace/.env.cloud_mode 2>/dev/null || true
fi

if [ -f /workspace/.devcontainer/opencode.json ]; then
    cp /workspace/.devcontainer/opencode.json "${HOME}/.opencode/opencode.json" 2>/dev/null && echo "Regenerated opencode config" || echo "WARNING: Failed to copy opencode config"
fi

if [ "$PROFILE" = "local" ]; then
    if [ -f /workspace/.devcontainer/scripts/write-bifrost-config.sh ]; then
        bash /workspace/.devcontainer/scripts/write-bifrost-config.sh 2>/dev/null || echo "WARNING: bifrost config generation failed"
    fi
fi

if pgrep -f "opencode serve" > /dev/null 2>&1; then
    echo "Restarting opencode..."
    pkill -f "opencode serve" 2>/dev/null || true
    sleep 2
    if command -v opencode >/dev/null 2>&1; then
        nohup opencode serve --port 4096 --config "${HOME}/.opencode/opencode.json" --password-file "${HOME}/.opencode/password" > "${HOME}/.cache/opencode.log" 2>&1 &
        echo "Restarted opencode"
    fi
fi

if [ "$PROFILE" = "local" ]; then
    if pgrep -f "bifrost" > /dev/null 2>&1; then
        echo "Restarting bifrost..."
        pkill -f "bifrost" 2>/dev/null || true
        sleep 2
        if command -v bifrost >/dev/null 2>&1 && [ -f "${HOME}/.config/bifrost/config.json" ]; then
            nohup bifrost --config "${HOME}/.config/bifrost/config.json" --port 8082 --host 0.0.0.0 > "${HOME}/.cache/llm-lab/bifrost.log" 2>&1 &
            sleep 3
            bash /workspace/.devcontainer/scripts/reconcile-bifrost.sh 2>/dev/null || echo "WARNING: bifrost reconciliation failed"
        fi
    fi
fi

echo "Profile switched to: $PROFILE"
echo ""
echo "To apply changes immediately in running container:"
echo "  source /workspace/.env.cloud_mode"
echo ""
echo "Or restart container:"
echo "  devcontainer up --workspace-folder ."

exit 0