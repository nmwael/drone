#!/bin/bash
set -euo pipefail

echo "=== Reconciling Bifrost config store ==="

BIFROST_URL="http://localhost:8082"
SETUP_TOKEN="${BIFROST_SETUP_TOKEN:-}"

# Wait for bifrost to be ready
for i in {1..30}; do
    if curl -sf "$BIFROST_URL/health" > /dev/null 2>&1; then
        break
    fi
    sleep 1
done

if [ -z "$SETUP_TOKEN" ]; then
    echo "Warning: BIFROST_SETUP_TOKEN not set, skipping reconciliation"
    exit 0
fi

# Get current providers from config store
echo "Fetching current providers..."
PROVIDERS=$(curl -sf -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/providers" | jq -r '.providers[].name' 2>/dev/null || echo "")

# Get expected providers from stack.json
EXPECTED_PROVIDERS=""
if [ -f "/workspace/.devcontainer/stack.json" ]; then
    EXPECTED_PROVIDERS=$(jq -r '.models[] | select(.provider != "local") | .provider' /workspace/.devcontainer/stack.json | sort -u | tr '\n' ' ')
fi

# Delete stale providers
if [ -n "$PROVIDERS" ]; then
    for provider in $PROVIDERS; do
        if [[ ! " $EXPECTED_PROVIDERS " =~ " $provider " ]]; then
            echo "Deleting stale provider: $provider"
            curl -sf -X DELETE -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/providers/$provider" || true
        fi
    done
fi

# Verify keys
echo "Verifying keys..."
curl -sf -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/keys" | jq '.'

echo "Bifrost reconciliation complete"