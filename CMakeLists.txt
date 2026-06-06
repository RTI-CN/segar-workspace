cmake_minimum_required(VERSION 3.10)
project(segar_examples VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Werror")
#set(CMAKE_CXX_FLAGS_RELEASE "-g")
# Set output directory
#download third party
include(${CMAKE_SOURCE_DIR}/cmake/GetThirdParty.cmake)
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
if(NOT DEFINED DEPS_FILE)
    set(DEPS_FILE "depend_libs.txt")
    message(STATUS "DEPS_FILE not set, using default: ${DEPS_FILE}")
endif()
load_dependencies("${PLATFORM_NAME}" "${DEPS_FILE}")

# Set install prefix to the output directory
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/output" CACHE PATH "Installation directory" FORCE)
else()
    # If user provides an install path, normalize it to an absolute path
    get_filename_component(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
set(THIRD_PARTY_PREFIX "${CMAKE_SOURCE_DIR}/install/${PLATFORM_NAME}/third_party" CACHE PATH "Tool installation prefix")
list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_PREFIX}")
find_package(RtiSegar REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(SEGAR_PYTHON_VERSION
    "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
set(SEGAR_PYTHON_PATH
    "lib/python${SEGAR_PYTHON_VERSION}/site-packages/segar/python")

# segar-transform ships SegarTransformConfig.cmake with find_dependency(RtiSegar), which
# re-runs FindRtiSegar and collides with the existing rti_segar target. Load the exported
# targets directly after RtiSegar is initialized.
set(_SEGAR_TRANSFORM_TARGETS
    "${THIRD_PARTY_PREFIX}/lib/cmake/SegarTransform/SegarTransformTargets.cmake")
if(NOT EXISTS "${_SEGAR_TRANSFORM_TARGETS}")
  message(FATAL_ERROR
    "segar-transform CMake exports not found at ${_SEGAR_TRANSFORM_TARGETS}. "
    "Ensure depend_libs.txt includes segar-transform and dependencies are installed.")
endif()
include("${_SEGAR_TRANSFORM_TARGETS}")

# Build switch for integration test module.
option(ENABLE_INTEGRATION_TEST "build segar integration tests" OFF)

add_subdirectory(src)
add_subdirectory(src_python)
install_dependencies("${PLATFORM_NAME}" "${CMAKE_INSTALL_PREFIX}/third_party")
install(PROGRAMS
  ${CMAKE_SOURCE_DIR}/scripts/start_all.sh
  ${CMAKE_SOURCE_DIR}/scripts/stop_all.sh
  ${CMAKE_SOURCE_DIR}/scripts/check_all.sh
  ${CMAKE_SOURCE_DIR}/scripts/run_segar_cli_test.sh
  DESTINATION ${CMAKE_INSTALL_PREFIX})

install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/)

set(_LAUNCH_EXAMPLE_GEN_DIR "${CMAKE_BINARY_DIR}/gen/launch_example")
file(MAKE_DIRECTORY "${_LAUNCH_EXAMPLE_GEN_DIR}")
configure_file(
  "${CMAKE_SOURCE_DIR}/src/launch_example/example.launch.in"
  "${_LAUNCH_EXAMPLE_GEN_DIR}/example.launch"
  @ONLY
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/launch_example/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/launch_example
        PATTERN "*.in" EXCLUDE)
install(FILES "${_LAUNCH_EXAMPLE_GEN_DIR}/example.launch"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/launch_example)
