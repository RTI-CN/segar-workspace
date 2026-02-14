#!/bin/bash

# Shared build script
# Usage: build_base.sh <BUILD_DIR_NAME> <TOOLCHAIN_FILE> [PLATFORM_NAME] [BUILD_TYPE]

set -e

# Validate arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <BUILD_DIR_NAME> <TOOLCHAIN_FILE> [PLATFORM_NAME] [BUILD_TYPE]"
    echo "Example: $0 build_x86 cmake/x86_64-linux-gcc_9.5.0.toolchain.cmake x86_64 Release"
    echo "Example: $0 build_orin cmake/aarch64-orin-gcc_13.2.0.toolchain.cmake orin Debug"
    exit 1
fi

BUILD_DIR_NAME="$1"
TOOLCHAIN_FILE_REL="$2"
PLATFORM_NAME="$3"
BUILD_TYPE="${4:-Release}"  # Defaults to Release

# Resolve project root from script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

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

cmake "${CMAKE_ARGS[@]}"

# Build
cmake --build . -- -j$(nproc)

# Install to output directory
cmake --install .

echo "Build completed. Executables are under: $BUILD_DIR/output"
