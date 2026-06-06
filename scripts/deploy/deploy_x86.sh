#! /bin/bash

# deploy segar workspace to x86_64
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

install_dir="/opt/robot_lab/segar-workspace"
if [ -n "$1" ]; then
    install_dir="$1"
fi
if [ "$install_dir" = "/opt/robot_lab/segar-workspace" ]; then
    echo "deploy segar workspace to $install_dir, need sudo"
    exec sudo bash "$SCRIPT_DIR/deploy_base.sh" x86_64 "$install_dir"
else
    echo "deploy segar workspace to $install_dir, not need sudo"
    exec bash "$SCRIPT_DIR/deploy_base.sh" x86_64 "$install_dir"
fi
