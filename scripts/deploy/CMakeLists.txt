cmake_minimum_required(VERSION 3.10)
project(segar_workspace-deploy VERSION 1.0.0 LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/../../cmake/GetThirdParty.cmake)
# Load dependencies manually; pass platform and dependency file path
# If PLATFORM_NAME is defined, derive PLATFORM from PLATFORM_NAME
if(DEFINED PLATFORM_NAME)
    if(PLATFORM_NAME STREQUAL "orin")
        set(PLATFORM "aarch64")
    elseif(PLATFORM_NAME STREQUAL "x86_64")
        set(PLATFORM "x86_64")
    else()
        # Default to PLATFORM_NAME when no alias mapping is matched
        set(PLATFORM "${PLATFORM_NAME}")
    endif()
    message(STATUS "PLATFORM_NAME is ${PLATFORM_NAME}, derived PLATFORM: ${PLATFORM}")
elseif(NOT DEFINED PLATFORM)
    set(PLATFORM "x86_64")
    set(PLATFORM_NAME "x86_64")
    message(STATUS "PLATFORM and PLATFORM_NAME are not defined, using default: ${PLATFORM}")
else()
    # If only PLATFORM is defined, use it as PLATFORM_NAME
    set(PLATFORM_NAME "${PLATFORM}")
    message(STATUS "PLATFORM is ${PLATFORM}, using as PLATFORM_NAME: ${PLATFORM_NAME}")
endif()

# 依赖列表：默认 segar-workspace/depend_libs.txt；可传入 -DDEPS_FILE=/path/to/depend_libs.txt
if(NOT DEFINED DEPS_FILE OR DEPS_FILE STREQUAL "")
    set(DEPS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../depend_libs.txt")
endif()
message(STATUS "Dependency list file (DEPS_FILE): ${DEPS_FILE}")
if(DEFINED INSTALL_DIR)
    load_dependencies("${PLATFORM_NAME}" "${DEPS_FILE}" INSTALL_DIR "${INSTALL_DIR}")
else()
    load_dependencies("${PLATFORM_NAME}" "${DEPS_FILE}")
endif()


