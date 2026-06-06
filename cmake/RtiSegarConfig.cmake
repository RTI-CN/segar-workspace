#### RtiSegarConfig.cmake — compatibility package: self-contained imports + optional Conan transitive deps
#
# Default imported targets remain rti::* for compatibility with existing projects.
# For segar::* names, see the ALIAS block at the end (CMake >= 3.19).
#
# To resolve transitive deps (e.g. gflags/protobuf) after find_package(RtiSegar), the consumer should
# already have the Conan toolchain on CMAKE_PREFIX_PATH (generators). This file then calls find_package below.
# Layout matches SegarTransform: <prefix>/lib/cmake/RtiSegar/RtiSegarConfig.cmake; install root is three levels up.

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(_rti_segar_include_dir "${PACKAGE_PREFIX_DIR}/include")
set(_rti_segar_lib_dir "${PACKAGE_PREFIX_DIR}/lib")

macro(_rti_segar_import_shared target_name so_basename)
  if(NOT TARGET "${target_name}")
    set(_so_path "${_rti_segar_lib_dir}/${so_basename}")
    if(NOT EXISTS "${_so_path}")
      message(FATAL_ERROR "RtiSegar: missing shared library: ${_so_path}")
    endif()
    add_library("${target_name}" SHARED IMPORTED)
    set_target_properties("${target_name}" PROPERTIES
      IMPORTED_LOCATION "${_so_path}"
      INTERFACE_INCLUDE_DIRECTORIES "${_rti_segar_include_dir}"
    )
  endif()
endmacro()



if(EXISTS "${_rti_segar_lib_dir}/librecord.so")
  _rti_segar_import_shared(rti::record "librecord.so")
endif()

if(EXISTS "${_rti_segar_lib_dir}/libtf2_segar.so")
  _rti_segar_import_shared(rti::segar_tf2 "libtf2_segar.so")
  # Link against libtf2.so from the same prefix
  set_property(TARGET rti::segar_tf2 APPEND PROPERTY 
    INTERFACE_LINK_LIBRARIES "${_rti_segar_lib_dir}/libtf2.so"
  )
endif()

set(RtiSegar_FOUND TRUE)
set(RtiSegar_INCLUDE_DIRS "${_rti_segar_include_dir}")
set(RtiSegar_LIBRARY_DIRS "${_rti_segar_lib_dir}")
set(RtiSegar_LIBRARIES rti::segar)
find_package(Rti3rd REQUIRED)
find_package(segar REQUIRED) 
find_package(SegarTransform REQUIRED)
find_package(msg_tool REQUIRED)
find_package(usr_msg REQUIRED)

# ---- ALIAS: segar::* and rti::* map to the same imported targets (CMake >= 3.19)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
  if(TARGET segar::segar AND NOT TARGET rti::segar)
    add_library(rti::segar ALIAS segar::segar)
  endif()
  if(TARGET msg_tool::msg_tool AND NOT TARGET rti::msg_tool)
    add_library(rti::msg_tool ALIAS msg_tool::msg_tool)
  endif()
  if(TARGET usr_msg::usr_msg AND NOT TARGET rti::usr_msg)
    add_library(rti::usr_msg ALIAS usr_msg::usr_msg)
  endif()
  if(TARGET record::record AND NOT TARGET segar::record)
    add_library(segar::record ALIAS record::record)
  endif()
endif()

unset(_rti_segar_include_dir)
unset(_rti_segar_lib_dir)
