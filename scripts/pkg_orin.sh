#!/usr/bin/env bash
# Orin packaging script

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PLATFORM_NAME="orin"
BUILD_DIR="build_orin"

# Invoke base packaging script
"${SCRIPT_DIR}/pkg_base.sh" "${PLATFORM_NAME}" "${BUILD_DIR}"
