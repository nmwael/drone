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
