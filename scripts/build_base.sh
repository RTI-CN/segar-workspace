#!/usr/bin/env bash

# Shared build script
# Usage: build_base.sh <BUILD_DIR_NAME> <TOOLCHAIN_FILE> [PLATFORM_NAME] [BUILD_TYPE] [CMAKE_ARGS...]

set -e

# Validate arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <BUILD_DIR_NAME> <TOOLCHAIN_FILE> [PLATFORM_NAME] [BUILD_TYPE] [CMAKE_ARGS...]"
    echo "Example: $0 build_x86 cmake/x86_64-native.toolchain.cmake x86_64 Release"
    echo "Example: $0 build_orin cmake/aarch64-orin-gcc_11.4.0.toolchain.cmake orin Debug"
    exit 1
fi

BUILD_DIR_NAME="$1"
TOOLCHAIN_FILE_REL="$2"
PLATFORM_NAME="$3"
BUILD_TYPE="${4:-Release}"  # Defaults to Release
EXTRA_CMAKE_ARGS=("${@:5}")

# Resolve project root from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Keep command-line downloads on the same proxy path as the desktop/browser.
# Clash/Mihomo on this machine exposes HTTP CONNECT at 127.0.0.1:7897.
if [ -z "${http_proxy:-}" ] && [ -n "${HTTP_PROXY:-}" ]; then
    export http_proxy="$HTTP_PROXY"
fi
if [ -z "${https_proxy:-}" ] && [ -n "${HTTPS_PROXY:-}" ]; then
    export https_proxy="$HTTPS_PROXY"
fi
if [ -z "${HTTP_PROXY:-}" ] && [ -n "${http_proxy:-}" ]; then
    export HTTP_PROXY="$http_proxy"
fi
if [ -z "${HTTPS_PROXY:-}" ] && [ -n "${https_proxy:-}" ]; then
    export HTTPS_PROXY="$https_proxy"
fi
if [ -z "${http_proxy:-}" ] && [ -z "${https_proxy:-}" ] && nc -z 127.0.0.1 7897 >/dev/null 2>&1; then
    export http_proxy="http://127.0.0.1:7897"
    export https_proxy="http://127.0.0.1:7897"
    export HTTP_PROXY="$http_proxy"
    export HTTPS_PROXY="$https_proxy"
fi

# Build the full toolchain file path
if [[ "$TOOLCHAIN_FILE_REL" == /* ]]; then
    # Absolute path
    TOOLCHAIN_FILE="$TOOLCHAIN_FILE_REL"
else
    # Relative path
    TOOLCHAIN_FILE="$PROJECT_ROOT/$TOOLCHAIN_FILE_REL"
fi

# Check toolchain file existence when provided
if [ -n "$TOOLCHAIN_FILE_REL" ] && [ "$TOOLCHAIN_FILE_REL" != "none" ]; then
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        echo "Error: toolchain file not found: $TOOLCHAIN_FILE"
        exit 1
    fi
fi

# Create build directory
BUILD_DIR="$PROJECT_ROOT/$BUILD_DIR_NAME"
mkdir -p "$BUILD_DIR"

# Enter build directory
cd "$BUILD_DIR"

# Set install prefix to build_dir/output
INSTALL_PREFIX="$BUILD_DIR/output"

# Run CMake configure
CMAKE_ARGS=(
    "$PROJECT_ROOT"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
)

# Use toolchain file when provided
if [ -n "$TOOLCHAIN_FILE_REL" ] && [ "$TOOLCHAIN_FILE_REL" != "none" ] && [ -f "$TOOLCHAIN_FILE" ]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE")
fi

# Pass PLATFORM_NAME when provided
if [ -n "$PLATFORM_NAME" ]; then
    CMAKE_ARGS+=(-DPLATFORM_NAME="$PLATFORM_NAME")
fi

CMAKE_ARGS+=("${EXTRA_CMAKE_ARGS[@]}")

cmake "${CMAKE_ARGS[@]}"

# Build
cmake --build . -- -j$(nproc)

# Install to output directory
cmake --install .

echo "Build completed. Executables are under: $BUILD_DIR/output"
