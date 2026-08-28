#!/bin/bash
# pixi-init.sh — bootstrap pixi + install the CxxKit pixi environment
#
# Ported from the main project (ms_remote_control_station/scripts/pixi-init.sh):
#   - same pinned pixi version + gitee mirror fallback (pixi-install.sh)
#   - same lock/install flow with activation cache
#   - DROPPED: pnpm install (no web frontend here) and pixi-pack /
#     pixi-unpack-tool (environment packing for distribution — CxxKit does
#     not ship env bundles; the main project owns that pipeline)
#
# Usage:
#   bash scripts/pixi-init.sh            # bootstrap + lock + install
#   bash scripts/pixi-init.sh --root-dir /path   # operate on another manifest dir

if [ -n "$BASH_SOURCE" ]; then
    SCRIPT_PATH="$BASH_SOURCE"
elif [ -n "$ZSH_VERSION" ]; then
    SCRIPT_PATH="${(%):-%x}"
else
    SCRIPT_PATH="$0"
fi
[ -z "$SCRIPT_PATH" ] && SCRIPT_PATH="$0"
if __dir="$(cd -- "$(dirname -- "$SCRIPT_PATH" 2>/dev/null)" && pwd -P 2>/dev/null)"; then
    SCRIPT_DIR="$__dir"
elif __dir="$(cd -- "$(dirname -- "$0")" && pwd -P 2>/dev/null)"; then
    SCRIPT_DIR="$__dir"
else
    SCRIPT_DIR="$(pwd -P 2>/dev/null || echo "/tmp")"
fi
# fail fast: propagate errors to calling scripts
set -euo pipefail

arg_root_dir="$(realpath "${SCRIPT_DIR}/..")"
while [ $# -gt 0 ]; do
    case "$1" in
        --root-dir)
            if [ -n "$2" ]; then
                mkdir -p "$2"
                arg_root_dir="$(realpath "$2")"
                shift 2
            else
                echo "Error: Invalid root dir."
                exit 1
            fi
            ;;
        *)
            echo "Error: Unknown parameter $1"
            exit 1
            ;;
    esac
done

PIXI_ARCH=$(uname -m)
export PIXI_VERSION="0.67.2"
export PATH="$HOME/.pixi/bin:$PATH"
NEED_INSTALL=false
if command -v pixi >/dev/null 2>&1; then
    CURRENT_PIXI_VERSION=$(pixi --version 2>/dev/null | grep -oP 'pixi \K[0-9.]+' || echo "unknown")
    if [ "${CURRENT_PIXI_VERSION}" = "${PIXI_VERSION}" ]; then
        echo "[INFO] pixi ${PIXI_VERSION} is already installed"
    else
        echo "[INFO] pixi version mismatch (installed: ${CURRENT_PIXI_VERSION:-unknown}, required: ${PIXI_VERSION}), reinstalling..."
        NEED_INSTALL=true
    fi
else
    echo "[INFO] pixi not installed, installing pixi ${PIXI_VERSION}..."
    NEED_INSTALL=true
fi

if [ "${NEED_INSTALL}" = "true" ]; then
    export PIXI_REPOURL="https://gitee.com/chengxuewen-github/pixi"
    bash "${SCRIPT_DIR}/pixi-install.sh"
    if [ $? -ne 0 ]; then
        echo "[ERROR] pixi installation failed"
        exit 1
    fi
    if [ -f "$HOME/.zshrc" ]; then
        source "$HOME/.zshrc"
    elif [ -f "$HOME/.bashrc" ]; then
        source "$HOME/.bashrc"
    fi
    # Re-export PATH after reinstall
    export PATH="$HOME/.pixi/bin:$PATH"
fi

# install the pixi environment (nodejs for the opencode toolchain)
export PIXI_CACHE_DIR="${arg_root_dir}/.pixi-cache"
echo "Enter pixi environment dir ${arg_root_dir}..."
cd "${arg_root_dir}" || { echo "Unable to enter directory: ${arg_root_dir}"; exit 1; }
echo "Start pixi lock..."
pixi lock || { echo "[ERROR] pixi lock failed"; exit 1; }
echo "Start pixi install..."
pixi install --use-environment-activation-cache || { echo "[ERROR] pixi install failed"; exit 1; }

echo "[INFO] pixi environment ready: $(pixi run node --version)"
