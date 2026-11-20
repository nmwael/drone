#!/bin/bash
# Agent Status Monitor - Persistent status line for OpenCode agents
# Run this in a tmux pane or add to PS1 for continuous monitoring

set +e

CACHE_DIR="${HOME}/.cache"
STATUS_FILE="${CACHE_DIR}/agent-status.json"
LOG_FILE="${CACHE_DIR}/agent-status.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Initialize status file
init_status() {
    mkdir -p "${CACHE_DIR}"
    if [ ! -f "${STATUS_FILE}" ]; then
        echo '{"agents":{},"updated":0}' > "${STATUS_FILE}"
    fi
}

# Update agent status
update_agent() {
    local agent_id=$1
    local agent_type=$2
    local status=$3
    local description=$4
    local pid=$5
    
    init_status
    
    # Use jq to update JSON
    local timestamp=$(date +%s)
    local tmp=$(mktemp)
    
    jq --arg id "$agent_id" \
       --arg type "$agent_type" \
       --arg status "$status" \
       --arg desc "$description" \
       --arg pid "$pid" \
       --arg ts "$timestamp" \
       '.agents[$id] = {"type": $type, "status": $status, "description": $desc, "pid": ($pid|tonumber), "updated": ($ts|tonumber)} | .updated = ($ts|tonumber)' \
       "${STATUS_FILE}" > "${tmp}" && mv "${tmp}" "${STATUS_FILE}"
}

# Remove agent
remove_agent() {
    local agent_id=$1
    init_status
    local tmp=$(mktemp)
    jq --arg id "$agent_id" 'del(.agents[$id]) | .updated = (now|floor)' "${STATUS_FILE}" > "${tmp}" && mv "${tmp}" "${STATUS_FILE}"
}

# Get session info from opencode CLI
get_sessions() {
    opencode session list --format json 2>/dev/null || echo '[]'
}

# Get subagent count (child sessions) - check for subagent entries in status file
get_subagent_count() {
    # Count agents with type in (architect, coder, researcher, reviewer) and status != completed
    jq -r '.agents | to_entries[] | select(.value.type | IN("architect","coder","researcher","reviewer")) | .key' "${STATUS_FILE}" 2>/dev/null | wc -l
}

# Get agent status for display
get_status_line() {
    init_status
    
    # Check opencode server (port 4096 or 4097)
    local opencode_status="⬤"
    local opencode_color="${RED}"
    local server_port=""
    for port in 4096 4097; do
        if curl -sf --max-time 1 "http://localhost:${port}/health" > /dev/null 2>&1; then
            opencode_status="⬤"
            opencode_color="${GREEN}"
            server_port="${port}"
            break
        fi
    done
    
    # Check active sessions from opencode CLI
    local sessions_json=$(get_sessions)
    local session_count=$(echo "${sessions_json}" | jq 'length' 2>/dev/null || echo "0")
    
    # Get subagent count from status file
    local subagent_count=$(get_subagent_count)
    
    # Build status line
    local line=""
    line+="${CYAN}${BOLD}▣ AGENTS${NC} "
    line+="${opencode_color}opencode${NC}:${GREEN}${opencode_status}${NC} "
    if [ -n "${server_port}" ]; then
        line+="${BLUE}port${NC}:${server_port} "
    fi
    line+="${BLUE}sessions${NC}:${session_count} "
    line+="${MAGENTA}subagents${NC}:${subagent_count} "
    
    # Add specific agent statuses from status file
    local agent_info=$(jq -r '.agents | to_entries[] | "\(.key)|\(.value.type)|\(.value.status)|\(.value.description)|\(.value.pid)"' "${STATUS_FILE}" 2>/dev/null)
    
    if [ -n "${agent_info}" ]; then
        line+="| "
        while IFS='|' read -r aid atype astatus adesc apid; do
            local color="${YELLOW}"
            case "${astatus}" in
                "running") color="${GREEN}" ;;
                "completed") color="${BLUE}" ;;
                "failed") color="${RED}" ;;
                "idle") color="${CYAN}" ;;
            esac
            line+="${color}${atype}:${astatus}${NC} "
        done <<< "${agent_info}"
    fi
    
    # Add network status
    local esp_ip=$(ip route get 8.8.8.8 2>/dev/null | grep -oP 'src \K\S+' || echo "N/A")
    line+="| ${YELLOW}ESP${NC}:${esp_ip} "
    
    echo -e "${line}"
}

# Watch mode - continuous update
watch_mode() {
    echo "Starting agent status monitor (Ctrl+C to exit)..."
    while true; do
        clear
        echo "═══════════════════════════════════════════════════════════════════"
        echo -e "$(get_status_line)"
        echo "═══════════════════════════════════════════════════════════════════"
        echo ""
        echo "OpenCode Server: http://localhost:${server_port:-4096}"
        echo "Logs: ${LOG_FILE}"
        echo ""
        echo "Active OpenCode Sessions:"
        get_sessions | jq -r '.[] | "  \(.id)  \(.title)"' 2>/dev/null || echo "  (none or CLI unavailable)"
        echo ""
        echo "Tracked Subagents (from status file):"
        jq -r '.agents | to_entries[] | select(.value.type | IN("architect","coder","researcher","reviewer")) | "  \(.key)  \(.value.type)  \(.value.status)  \(.value.description)"' "${STATUS_FILE}" 2>/dev/null || echo "  (none)"
        sleep 3
    done
}

# One-shot status for PS1 integration
ps1_status() {
    init_status
    local sessions_json=$(get_sessions)
    local session_count=$(echo "${sessions_json}" | jq 'length' 2>/dev/null || echo "0")
    local subagent_count=$(get_subagent_count)
    
    local opencode_indicator="●"
    for port in 4096 4097; do
        if curl -sf --max-time 1 "http://localhost:${port}/health" > /dev/null 2>&1; then
            opencode_indicator="●"
            break
        fi
    done
    
    echo "⚡${session_count}🤖${subagent_count}"
}

# Main command handling
case "${1:-status}" in
    "watch"|"w")
        watch_mode
        ;;
    "ps1")
        ps1_status
        ;;
    "update")
        update_agent "$2" "$3" "$4" "$5" "$6"
        ;;
    "remove")
        remove_agent "$2"
        ;;
    "status"|*)
        get_status_line
        ;;
esac