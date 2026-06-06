#!/usr/bin/env bash

# x86 build script
# Usage: build_x86.sh [-d] [-r] [-ra] [-it] [-ut] [-h]
#   -d: Build Debug variant (default: Release)
#   -r: Remove build_x86 directory
#   -ra: Remove build_x86 and install/x86_64 directories
#   -it: Enable integration_test module
#   -ut: Run workspace example smoke tests after successful build/install
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
RUN_EXAMPLE_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Usage: $0 [-d] [-r] [-ra] [-it] [-ut] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_x86 directory"
            echo "  -ra: Remove build_x86 and install/x86_64 directories"
            echo "  -it: Enable integration_test module"
            echo "  -ut: Run workspace example smoke tests after successful build/install"
            echo "  -h:  Show this help message"
            exit 0
            ;;
        -it)
            ENABLE_INTEGRATION_TEST=true
            shift
            ;;
        -ut)
            RUN_EXAMPLE_TESTS=true
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
            echo "Usage: $0 [-d] [-r] [-ra] [-it] [-ut] [-h]"
            echo "  -d:  Build Debug variant (default: Release)"
            echo "  -r:  Remove build_x86 directory"
            echo "  -ra: Remove build_x86 and install/x86_64 directories"
            echo "  -it: Enable integration_test module"
            echo "  -ut: Run workspace example smoke tests after successful build/install"
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
EXTRA_CMAKE_ARGS=()
if [ "$ENABLE_INTEGRATION_TEST" = true ]; then
    EXTRA_CMAKE_ARGS+=(-DENABLE_INTEGRATION_TEST=ON)
fi
"$SCRIPT_DIR/build_base.sh" \
    "build_x86" \
    "cmake/x86_64-native.toolchain.cmake" \
    "x86_64" \
    "$BUILD_TYPE" \
    "${EXTRA_CMAKE_ARGS[@]}"

if [ "$RUN_EXAMPLE_TESTS" = true ]; then
    "$SCRIPT_DIR/run_examples_smoke.sh" "$PROJECT_ROOT/build_x86/output"
fi

if [ "$ENABLE_INTEGRATION_TEST" = true ]; then
    echo "Running integration tests..."
    cd ${PROJECT_ROOT} || exit
    ./build_x86/output/integration_test/run_integration_test.sh
    ./build_x86/output/integration_test/run_performance_test.sh
    INTEGRATION_TEST_RESULT=$?
    if [ $INTEGRATION_TEST_RESULT -ne 0 ]; then
        echo "Integration tests failed"
        exit $INTEGRATION_TEST_RESULT
    fi
fi



echo "Target architecture: x86_64"
echo "Build type: $BUILD_TYPE"
