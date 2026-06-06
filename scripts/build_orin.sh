#!/usr/bin/env bash

# ARM cross-compilation script (for Orin platform)
# Usage: build_orin.sh [-d] [-r] [-ra] [-it] [-h]
#   -d: Build Debug variant (default: Release)
#   -r: Remove build_orin directory
#   -ra: Remove build_orin and install/orin directories
#   -it: Enable integration_test module
#   -h: Show help

set -e

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
CLEAN_ALL=false
ENABLE_INTEGRATION_TEST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Usage: $0 [-d] [-r] [-ra] [-it] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_orin directory"
            echo "  -ra: Remove build_orin and install/orin directories"
            echo "  -it: Enable integration_test module"
            echo "  -h:  Show this help message"
            exit 0
            ;;
        -it)
            ENABLE_INTEGRATION_TEST=true
            shift
            ;;
        -d)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r)
            CLEAN_BUILD=true
            shift
            ;;
        -ra)
            CLEAN_BUILD=true
            CLEAN_ALL=true
            shift
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [-d] [-r] [-ra] [-it] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_orin directory"
            echo "  -ra: Remove build_orin and install/orin directories"
            echo "  -it: Enable integration_test module"
            echo "  -h:  Show this help message"
            exit 1
            ;;
    esac
done

# Remove build directory
if [ "$CLEAN_BUILD" = true ]; then
    BUILD_DIR="$PROJECT_ROOT/build_orin"
    if [ -d "$BUILD_DIR" ]; then
        echo "Removing build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    else
        echo "Build directory does not exist: $BUILD_DIR"
    fi
fi

# Remove install directory
if [ "$CLEAN_ALL" = true ]; then
    INSTALL_DIR="$PROJECT_ROOT/install/orin"
    if [ -d "$INSTALL_DIR" ]; then
        echo "Removing install directory: $INSTALL_DIR"
        rm -rf "$INSTALL_DIR"
    else
        echo "Install directory does not exist: $INSTALL_DIR"
    fi
fi

# Note: build continues after cleanup by design.
# If cleanup-only behavior is needed, stop manually after -r/-ra or add a dedicated option.

# Invoke shared build script
EXTRA_CMAKE_ARGS=()
if [ "$ENABLE_INTEGRATION_TEST" = true ]; then
    EXTRA_CMAKE_ARGS+=(-DENABLE_INTEGRATION_TEST=ON)
fi
"$SCRIPT_DIR/build_base.sh" \
    "build_orin" \
    "cmake/aarch64-orin-gcc_11.4.0.toolchain.cmake" \
    "orin" \
    "$BUILD_TYPE" \
    "${EXTRA_CMAKE_ARGS[@]}"

echo "Target architecture: aarch64 (ARM64)"
echo "Build type: $BUILD_TYPE"
