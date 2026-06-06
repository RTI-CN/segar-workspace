# 交叉编译工具链配置文件
# 用于 aarch64-linux-gnu (ARM64) 架构，使用 Ubuntu 系统自带 GCC 11.4

# 使用系统 PATH 中的交叉编译工具（apt install gcc-11-aarch64-linux-gnu g++-11-aarch64-linux-gnu）
set(TOOLCHAIN_PREFIX "aarch64-linux-gnu")

# 设置系统信息
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 设置编译器（GCC 11.4）
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}-gcc-11")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}-g++-11")

# 设置工具链程序（binutils 无版本后缀）
set(CMAKE_AR "${TOOLCHAIN_PREFIX}-ar")
set(CMAKE_STRIP "${TOOLCHAIN_PREFIX}-strip")
set(CMAKE_RANLIB "${TOOLCHAIN_PREFIX}-ranlib")
set(CMAKE_NM "${TOOLCHAIN_PREFIX}-nm")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}-objdump")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}-objcopy")

# 设置查找程序的模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
