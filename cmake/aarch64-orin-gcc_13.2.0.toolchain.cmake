# Cross-compilation toolchain configuration
# For aarch64-none-linux-gnu (ARM64)

# Toolchain path
set(TOOLCHAIN_PATH "/opt/x-tools/aarch64-none-linux-gnu")
set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PATH}/bin/aarch64-none-linux-gnu")

# System settings
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compiler settings
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}-g++")

# Toolchain utilities
set(CMAKE_AR "${TOOLCHAIN_PREFIX}-ar")
set(CMAKE_STRIP "${TOOLCHAIN_PREFIX}-strip")
set(CMAKE_RANLIB "${TOOLCHAIN_PREFIX}-ranlib")
set(CMAKE_NM "${TOOLCHAIN_PREFIX}-nm")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}-objdump")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}-objcopy")

# Find path behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Optional: set sysroot if provided by the toolchain
# set(CMAKE_SYSROOT "${TOOLCHAIN_PATH}/aarch64-none-linux-gnu/sysroot")
