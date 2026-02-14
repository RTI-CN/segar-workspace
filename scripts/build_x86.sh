#!/bin/bash

# x86 build script
# Usage: build_x86.sh [-d] [-r] [-ra] [-h]
#   -d: Build Debug variant (default: Release)
#   -r: Remove build_x86 directory
#   -ra: Remove build_x86 and install/x86_64 directories
#   -h: Show help

set -e

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Parse arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
CLEAN_ALL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Usage: $0 [-d] [-r] [-ra] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_x86 directory"
            echo "  -ra: Remove build_x86 and install/x86_64 directories"
            echo "  -h:  Show this help message"
            exit 0
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
            echo "Usage: $0 [-d] [-r] [-ra] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_x86 directory"
            echo "  -ra: Remove build_x86 and install/x86_64 directories"
            echo "  -h:  Show this help message"
            exit 1
            ;;
    esac
done

# Remove build directory
if [ "$CLEAN_BUILD" = true ]; then
    BUILD_DIR="$PROJECT_ROOT/build_x86"
    if [ -d "$BUILD_DIR" ]; then
        echo "Removing build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    else
        echo "Build directory does not exist: $BUILD_DIR"
    fi
fi

# Remove install directory
if [ "$CLEAN_ALL" = true ]; then
    INSTALL_DIR="$PROJECT_ROOT/install/x86_64"
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
"$SCRIPT_DIR/build_base.sh" \
    "build_x86" \
    "cmake/x86_64-linux-gcc_9.5.0.toolchain.cmake" \
    "x86_64" \
    "$BUILD_TYPE"

echo "Target architecture: x86_64"
echo "Build type: $BUILD_TYPE"
