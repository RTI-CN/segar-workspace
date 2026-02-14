#!/usr/bin/env bash
# x86_64 packaging script

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PLATFORM_NAME="x86_64"
BUILD_DIR="build_x86"

# Invoke base packaging script
"${SCRIPT_DIR}/pkg_base.sh" "${PLATFORM_NAME}" "${BUILD_DIR}"
