#!/bin/bash
# Fetch model weights for local mode

set +e

echo "=== Fetching model weights ==="

MODELS_DIR="/workspace/models"
CACHE_DIR="${HOME}/.cache/llm-lab"

mkdir -p "$MODELS_DIR"
mkdir -p "$CACHE_DIR"

declare -A MODELS=(
    ["qwen2.5-coder-3b-q4_k_m.gguf"]="https://huggingface.co/Qwen/Qwen2.5-Coder-3B-GGUF/resolve/main/qwen2.5-coder-3b-q4_k_m.gguf"
    ["qwen2.5-coder-7b-q4_k_m.gguf"]="https://huggingface.co/Qwen/Qwen2.5-Coder-7B-GGUF/resolve/main/qwen2.5-coder-7b-q4_k_m.gguf"
    ["gemma-4-12b-ud-iq3_m.gguf"]="https://huggingface.co/bartowski/gemma-4-12b-GGUF/resolve/main/gemma-4-12b-UD-IQ3_M.gguf"
    ["phi-4-iq3_m.gguf"]="https://huggingface.co/microsoft/Phi-4-mini-GGUF/resolve/main/Phi-4-mini-Q4_K_M.gguf"
)

download_model() {
    local filename=$1
    local url=$2
    local filepath="$MODELS_DIR/$filename"
    local temp_file="${filepath}.tmp"

    if [ -f "$filepath" ]; then
        echo "✓ $filename already exists, skipping"
        return 0
    fi

    echo "Downloading $filename..."
    if curl -fL --retry 3 --retry-delay 5 -o "$temp_file" "$url"; then
        if file "$temp_file" 2>/dev/null | grep -q "GGUF"; then
            mv "$temp_file" "$filepath"
            echo "✓ Downloaded and verified $filename"
        else
            echo "✗ $filename is not a valid GGUF file"
            rm -f "$temp_file"
            return 1
        fi
    else
        echo "✗ Failed to download $filename"
        rm -f "$temp_file"
        return 1
    fi
}

for filename in "${!MODELS[@]}"; do
    download_model "$filename" "${MODELS[$filename]}" || echo "WARNING: Failed to download $filename, continuing..."
done

echo "=== Model fetch complete ==="
ls -lh "$MODELS_DIR" 2>/dev/null || true

exit 0