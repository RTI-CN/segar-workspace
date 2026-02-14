#!/usr/bin/env bash
# Base packaging script
# Args:
#   $1: PLATFORM_NAME - platform name (e.g., x86_64, orin)
#   $2: BUILD_DIR - build directory name (e.g., build_x86, build_orin)

set -e

PLATFORM_NAME="$1"
BUILD_DIR="$2"

# Derive normalized PLATFORM from PLATFORM_NAME
case "${PLATFORM_NAME}" in
    x86_64|amd64)
        PLATFORM="x86_64"
        ;;
    orin|aarch64|arm64)
        PLATFORM="aarch64"
        ;;
    *)
        # Fallback: use PLATFORM_NAME directly
        PLATFORM="${PLATFORM_NAME}"
        ;;
esac

# Resolve script directory
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SCRIPT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)  # Project root
OUTPUT_DIR="${SCRIPT_DIR}/${BUILD_DIR}/output"

# Check whether output directory exists
if [ ! -d "${OUTPUT_DIR}" ]; then
    echo "Error: ${OUTPUT_DIR} does not exist!"
    echo "Please run build script first to build the project."
    exit 1
fi

# Switch to project root
cd "${SCRIPT_DIR}"

# Read version from CMakeLists.txt
CMAKELISTS_FILE="${SCRIPT_DIR}/CMakeLists.txt"
if [ -f "${CMAKELISTS_FILE}" ]; then
    # Extract version from project(segar_examples VERSION x.x.x)
    VERSION_NAME=$(grep -E "^project\(segar_examples VERSION" "${CMAKELISTS_FILE}" | sed -E 's/.*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/' || echo "")
    if [ -z "${VERSION_NAME}" ]; then
        echo "Warning: Could not extract version from CMakeLists.txt, using default"
        VERSION_NAME="unknown"
    fi
else
    echo "Warning: CMakeLists.txt not found, using default version"
    VERSION_NAME="unknown"
fi

# Collect git metadata (for version file)
COMMIT_ID=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
FULL_COMMIT_ID=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
BUILD_TIME=$(date '+%Y-%m-%d %H:%M:%S %z')

# Try to resolve tag info
GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || git describe --tags 2>/dev/null || echo "")

# Get branch info
GIT_BRANCH=$(git branch --show-current 2>/dev/null || echo "")

# Sanitize version string for file names
CLEAN_VERSION_NAME=$(echo "${VERSION_NAME}" | sed 's/[^a-zA-Z0-9._-]/-/g')


# Compute MD5 for all binaries
BIN_MD5_LIST=""
BIN_COUNT=0

# Find all executables under bin directories
# Keep absolute path handling explicit
cd "${OUTPUT_DIR}" || exit 1
while IFS= read -r -d '' bin_file; do
    # Convert to absolute path
    abs_bin_file="${OUTPUT_DIR}/${bin_file}"
    if [ -f "${abs_bin_file}" ] && [ -x "${abs_bin_file}" ]; then
        # Relative to OUTPUT_DIR, strip leading ./
        rel_path="${bin_file#./}"
        md5_value=$(md5sum "${abs_bin_file}" | cut -d' ' -f1)
        if [ -z "${BIN_MD5_LIST}" ]; then
            BIN_MD5_LIST="${rel_path}: ${md5_value}"
        else
            BIN_MD5_LIST="${BIN_MD5_LIST}"$'\n'"${rel_path}: ${md5_value}"
        fi
        BIN_COUNT=$((BIN_COUNT + 1))
    fi
done < <(find . -type f -path "*/bin/*" -print0 2>/dev/null || echo -n '')
cd "${SCRIPT_DIR}" || exit 1

# Create version file
VERSION_FILE="${OUTPUT_DIR}/segar_examples_version.txt"
{
    echo "Build Time: ${BUILD_TIME}"
    echo "Version: ${VERSION_NAME}"
    echo "Platform: ${PLATFORM}"
    echo "Platform Name: ${PLATFORM_NAME}"
    echo "Commit ID: ${FULL_COMMIT_ID}"
    echo "Short Commit ID: ${COMMIT_ID}"
    echo "Branch: ${GIT_BRANCH}"
    echo "Tag: ${GIT_TAG}"
    echo ""
    echo "Bin Files MD5 (${BIN_COUNT} files):"
    if [ -n "${BIN_MD5_LIST}" ]; then
        echo "${BIN_MD5_LIST}"
    else
        echo "  No bin files found"
    fi
} > "${VERSION_FILE}"

echo "Version file created at ${VERSION_FILE}"
echo "---"
cat "${VERSION_FILE}"
echo "---"

# Build package name: segar_examples_<platform_name>_<version>
PACKAGE_NAME="segar_examples_${PLATFORM_NAME}_${CLEAN_VERSION_NAME}"
PACKAGE_FILE="${SCRIPT_DIR}/${BUILD_DIR}/${PACKAGE_NAME}.tgz"

# Enter build directory
cd "${SCRIPT_DIR}/${BUILD_DIR}"

# Create temporary directory and rename output to package name
TEMP_DIR=$(mktemp -d)
cp -r output "${TEMP_DIR}/${PACKAGE_NAME}"

# Create archive
echo "Creating package: ${PACKAGE_FILE}"
cd "${TEMP_DIR}"
tar -czf "${PACKAGE_FILE}" "${PACKAGE_NAME}"

# Cleanup temporary directory
rm -rf "${TEMP_DIR}"

# Show package info
PACKAGE_SIZE=$(du -h "${PACKAGE_FILE}" | cut -f1)
echo ""
echo "Package created successfully!"
echo "Package file: ${PACKAGE_FILE}"
echo "Package size: ${PACKAGE_SIZE}"
echo "Package name: ${PACKAGE_NAME}"
