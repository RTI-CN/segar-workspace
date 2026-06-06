#! /bin/bash

# deploy segar workspace for orin (cross-build deps on x86 host)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

install_dir="/opt/robot_lab/segar-workspace"
if [ -n "$1" ]; then
    install_dir="$1"
fi
if [ "$install_dir" = "/opt/robot_lab/segar-workspace" ]; then
    echo "deploy segar workspace to $install_dir, need sudo"
    exec sudo bash "$SCRIPT_DIR/deploy_base.sh" orin "$install_dir"
else
    echo "deploy segar workspace to $install_dir, not need sudo"
    exec bash "$SCRIPT_DIR/deploy_base.sh" orin "$install_dir"
fi
