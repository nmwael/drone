#!/bin/bash
# Reconcile Bifrost config store - remove stale providers

set +e

echo "=== Reconciling Bifrost config store ==="

BIFROST_URL="http://localhost:8082"
SETUP_TOKEN="${BIFROST_SETUP_TOKEN:-}"

for i in {1..30}; do
    if curl -sf --max-time 2 "$BIFROST_URL/health" > /dev/null 2>&1; then
        break
    fi
    sleep 1
done

if [ -z "$SETUP_TOKEN" ]; then
    echo "Warning: BIFROST_SETUP_TOKEN not set, skipping reconciliation"
    exit 0
fi

echo "Fetching current providers..."
PROVIDERS=$(curl -sf --max-time 5 -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/providers" 2>/dev/null | jq -r '.providers[].name' 2>/dev/null || echo "")

EXPECTED_PROVIDERS=""
if [ -f "/workspace/.devcontainer/stack.json" ]; then
    EXPECTED_PROVIDERS=$(jq -r '.models[] | select(.provider != "local") | .provider' /workspace/.devcontainer/stack.json 2>/dev/null | sort -u | tr '\n' ' ' || echo "")
fi

if [ -n "$PROVIDERS" ]; then
    for provider in $PROVIDERS; do
        if [[ ! " $EXPECTED_PROVIDERS " =~ " $provider " ]]; then
            echo "Deleting stale provider: $provider"
            curl -sf --max-time 5 -X DELETE -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/providers/$provider" 2>/dev/null || true
        fi
    done
fi

echo "Verifying keys..."
curl -sf --max-time 5 -H "X-Bifrost-Setup-Token: $SETUP_TOKEN" "$BIFROST_URL/api/keys" 2>/dev/null | jq '.' 2>/dev/null || echo "WARNING: Failed to verify keys"

echo "Bifrost reconciliation complete"

exit 0