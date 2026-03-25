# Cross-compilation toolchain configuration file
# For aarch64-linux-gnu (ARM64) architecture, use GCC 11.4 that comes with Ubuntu system

# Use the cross-compilation tool in the system PATH (apt install gcc-11-aarch64-linux-gnu g++-11-aarch64-linux-gnu)
set(TOOLCHAIN_PREFIX "aarch64-linux-gnu")

# Set system information
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Set up the compiler (GCC 11.4)
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}-gcc-11")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}-g++-11")

# Set the toolchain program (binutils has no version suffix)
set(CMAKE_AR "${TOOLCHAIN_PREFIX}-ar")
set(CMAKE_STRIP "${TOOLCHAIN_PREFIX}-strip")
set(CMAKE_RANLIB "${TOOLCHAIN_PREFIX}-ranlib")
set(CMAKE_NM "${TOOLCHAIN_PREFIX}-nm")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}-objdump")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}-objcopy")

# Set the search program mode
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
