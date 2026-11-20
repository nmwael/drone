#!/bin/bash
# Tmux-based Agent Monitor - Creates a persistent monitoring pane
# Run this once per tmux session to set up the monitor

SESSION_NAME="drone-dev"
MONITOR_PANE_TITLE="AGENT-MONITOR"

# Check if tmux session exists
if ! tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
    echo "Creating new tmux session: $SESSION_NAME"
    tmux new-session -d -s "$SESSION_NAME" -n "main" "cd /workspaces/drone/firmware/stanag-s3-simulator; bash"
fi

# Check if monitor pane exists
if ! tmux list-panes -t "$SESSION_NAME" -F "#{pane_title}" | grep -q "$MONITOR_PANE_TITLE"; then
    echo "Creating agent monitor pane..."
    # Split the main window vertically, 20% for monitor
    tmux split-window -t "$SESSION_NAME:main" -v -p 20 -d "bash -c 'cd /workspaces/drone; /workspaces/drone/.devcontainer/scripts/agent-status.sh watch'"
    tmux select-pane -t "$SESSION_NAME:main.1" -T "$MONITOR_PANE_TITLE"
    tmux select-pane -t "$SESSION_NAME:main.0"
fi

echo "Agent monitor ready in tmux session: $SESSION_NAME"
echo "Attach with: tmux attach -t $SESSION_NAME"