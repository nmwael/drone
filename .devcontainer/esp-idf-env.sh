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

# Add clangd (LSP) to PATH
export PATH="/home/vscode/.local/bin:$PATH"

# Agent Status Monitoring
export AGENT_STATUS_SCRIPT="/workspaces/drone/.devcontainer/scripts/agent-status.sh"
alias agents='bash "$AGENT_STATUS_SCRIPT" status'
alias agents-watch='bash "$AGENT_STATUS_SCRIPT" watch'
alias agents-ps1='bash "$AGENT_STATUS_SCRIPT" ps1'

# Add agent status to prompt (optional - uncomment to enable)
# export PS1='\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\] $(agents-ps1)\$ '

# Tmux Agent Monitor
alias agents-tmux='bash /workspaces/drone/.devcontainer/scripts/tmux-agent-monitor.sh'
alias ta='tmux attach -t drone-dev || bash /workspaces/drone/.devcontainer/scripts/tmux-agent-monitor.sh && tmux attach -t drone-dev'
