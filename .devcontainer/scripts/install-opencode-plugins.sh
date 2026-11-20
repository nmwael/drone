#!/bin/bash
# Install OpenCode plugins that work with v2.3.2+
# Runs during devcontainer postCreateCommand

set -e

echo "=== Installing OpenCode plugins ==="

# Create global opencode config directory
mkdir -p /home/vscode/.config/opencode

# Install @rashidrazak/opencode-status-line (working status line)
echo "Installing @rashidrazak/opencode-status-line..."
cd /home/vscode/.config/opencode
npm install @rashidrazak/opencode-status-line@1.2.0 --legacy-peer-deps 2>&1 | tail -5

# Add to global cli.json
cat > /home/vscode/.config/opencode/cli.json << 'EOF'
{
  "$schema": "https://opencode.ai/v2/cli.json",
  "plugins": [
    "@rashidrazak/opencode-status-line"
  ]
}
EOF

# Install opencode-subagent-statusline locally in project (dependency conflicts with global)
# The project .opencode/opencode.json will reference it via local path
echo "Installing opencode-subagent-statusline in project..."
cd /workspaces/drone/firmware/stanag-s3-simulator
mkdir -p .opencode/plugins
cd .opencode/plugins
npm install opencode-subagent-statusline@2.1.1 --legacy-peer-deps 2>&1 | tail -5

# Verify installations
echo ""
echo "=== Plugin verification ==="
echo "Global plugins:"
ls -la /home/vscode/.config/opencode/node_modules/ 2>/dev/null | grep -E "(opencode-status-line|opencode-subagent)" || echo "  (check npm install output above)"

echo ""
echo "Project plugins:"
ls -la /workspaces/drone/firmware/stanag-s3-simulator/.opencode/plugins/ 2>/dev/null | grep -E "(opencode-status-line|opencode-subagent)" || echo "  (check npm install output above)"

echo ""
echo "=== OpenCode plugins installation complete ==="