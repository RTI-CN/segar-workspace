# Shared helper: install config files and scripts
function(_install_example_resources TARGET_NAME EXAMPLE_NAME SUBDIR_NAME)
    # Install config directory (if present)
    if(EXISTS ${CMAKE_SOURCE_DIR}/src/${EXAMPLE_NAME}/${SUBDIR_NAME}/config)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/${EXAMPLE_NAME}/${SUBDIR_NAME}/config/
            DESTINATION ${EXAMPLE_NAME}/${TARGET_NAME}/config
            FILES_MATCHING
            PATTERN "*.*"
        )
    endif()
    
    # Install scripts from scripts directory (if present)
    file(GLOB SCRIPT_FILES "${CMAKE_SOURCE_DIR}/src/${EXAMPLE_NAME}/${SUBDIR_NAME}/scripts/*.sh")
    if(SCRIPT_FILES)
        install(PROGRAMS ${SCRIPT_FILES}
            DESTINATION ${EXAMPLE_NAME}/${TARGET_NAME}/scripts
        )
    endif()
endfunction()

function(add_example BINNAME SRC_FILES)
    add_executable(${BINNAME} ${SRC_FILES})
    target_link_libraries(${BINNAME} PRIVATE rti::segar ${GENERATED_IDL_OBJ_LIB})
    
    # Derive example and subdirectory names from CMAKE_CURRENT_LIST_DIR
    get_filename_component(SUBDIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${PARENT_DIR} NAME)
    
    # Install executable to ${EXAMPLE_NAME}/${BINNAME}/bin
    install(TARGETS ${BINNAME} DESTINATION ${EXAMPLE_NAME}/${BINNAME}/bin)
    
    # Install config files and scripts
    _install_example_resources(${BINNAME} ${EXAMPLE_NAME} ${SUBDIR_NAME})
endfunction()

function(add_example_lib BINNAME SRC_FILES)
    add_library(${BINNAME} SHARED ${SRC_FILES})
    target_link_libraries(${BINNAME} PRIVATE rti::segar ${GENERATED_IDL_OBJ_LIB})
    
    # Derive example and subdirectory names from CMAKE_CURRENT_LIST_DIR
    get_filename_component(SUBDIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${PARENT_DIR} NAME)
    
    # Install shared library to ${EXAMPLE_NAME}/${BINNAME}/lib
    install(TARGETS ${BINNAME} DESTINATION ${EXAMPLE_NAME}/${BINNAME}/lib)
    
    # Install config files and scripts
    _install_example_resources(${BINNAME} ${EXAMPLE_NAME} ${SUBDIR_NAME})
endfunction()

function(add_transform_example BINNAME SRC_FILES)
    add_executable(${BINNAME} ${SRC_FILES})
    # libtf2_segar.so may depend on Segar symbols not referenced from this TU; keep
    # rti::segar on the link line even with --as-needed (GNU ld).
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(${BINNAME} PRIVATE
            "LINKER:--no-as-needed")
    endif()
    # Link libtf2.so by path so we do not use the Segar::tf2 imported target's
    # INTERFACE_INCLUDE_DIRECTORIES (.../include/tf2), which shadows <time.h> with
    # tf2/time.h when system headers are included after Segar headers.
    target_link_libraries(${BINNAME} PRIVATE rti::segar rti::usr_msg  rti::segar_tf2)

    get_filename_component(SUBDIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${PARENT_DIR} NAME)

    install(TARGETS ${BINNAME} DESTINATION ${EXAMPLE_NAME}/${BINNAME}/bin)
    _install_example_resources(${BINNAME} ${EXAMPLE_NAME} ${SUBDIR_NAME})
endfunction()

function(add_usr_msg_example BINNAME SRC_FILES)
    add_executable(${BINNAME} ${SRC_FILES})
    target_link_libraries(${BINNAME} PRIVATE rti::segar rti::usr_msg)
    target_include_directories(${BINNAME} PRIVATE
        "${THIRD_PARTY_PREFIX}/include/usr_msg")

    get_filename_component(SUBDIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${PARENT_DIR} NAME)

    install(TARGETS ${BINNAME} DESTINATION ${EXAMPLE_NAME}/${BINNAME}/bin)
    _install_example_resources(${BINNAME} ${EXAMPLE_NAME} ${SUBDIR_NAME})
endfunction()

function(_ensure_generated_namespace_package PY_ROOT_DIR PACKAGE_NAME)
    set(_init_file "${PY_ROOT_DIR}/${PACKAGE_NAME}/__init__.py")
    if(NOT EXISTS "${_init_file}")
        return()
    endif()

    file(READ "${_init_file}" _init_content)
    if(_init_content MATCHES "extend_path\\(__path__, __name__\\)")
        return()
    endif()

    set(_patched_init
"from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

${_init_content}")
    file(WRITE "${_init_file}" "${_patched_init}")
    message(STATUS "Patched namespace package init: ${_init_file}")
endfunction()

find_file(USR_MSG_GENERATOR_CMAKE
    NAMES msg_tool_generate_code.cmake
    PATHS
        ${THIRD_PARTY_PREFIX}/lib/cmake/msg_tool
    NO_DEFAULT_PATH
)

if(USR_MSG_GENERATOR_CMAKE)
    include(${USR_MSG_GENERATOR_CMAKE})
    message(STATUS "Found msg_tool_generate_code: ${USR_MSG_GENERATOR_CMAKE}")
else()
    message(WARNING "msg_tool_generate_code.cmake not found, message generation will be skipped")
endif()

MsgToolCompile(${CMAKE_CURRENT_LIST_DIR}/type_src ${CMAKE_BINARY_DIR}/generate example_msg)
_ensure_generated_namespace_package("${CMAKE_BINARY_DIR}/generate/src/python" "segar")

include("${THIRD_PARTY_PREFIX}/lib/cmake/msg_tool/proto_generate_code.cmake")
SegarProtocCompile(${CMAKE_CURRENT_LIST_DIR}/type_src ${CMAKE_BINARY_DIR}/generate/proto example_proto)

set(GENERATED_PROTO_OBJ_LIB "")
if(TARGET example_proto)
    set(GENERATED_PROTO_OBJ_LIB example_proto)
endif()

set(EXAMPLE_PROTO_PYTHON_PACKAGE_DIR "${CMAKE_BINARY_DIR}/generate/proto/python/segar/proto")
if(EXISTS "${EXAMPLE_PROTO_PYTHON_PACKAGE_DIR}")
    file(WRITE "${EXAMPLE_PROTO_PYTHON_PACKAGE_DIR}/__init__.py"
"from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)
")
    install(DIRECTORY "${EXAMPLE_PROTO_PYTHON_PACKAGE_DIR}/"
        DESTINATION "${SEGAR_PYTHON_PATH}/../proto"
        FILES_MATCHING
        PATTERN "*_pb2.py"
        PATTERN "__init__.py")
endif()

# Compatibility alias for migrated modules that link against examples_msg.
if(TARGET example_msg AND NOT TARGET examples_msg)
    add_library(examples_msg ALIAS example_msg)
endif()

add_subdirectory(action_example)
add_subdirectory(component_example)
add_subdirectory(param_example)
add_subdirectory(service_example)
add_subdirectory(concurrent_example)
add_subdirectory(lifecycle_example)
add_subdirectory(topic_example)
add_subdirectory(zero_copy_example)
if(ENABLE_INTEGRATION_TEST)
    add_subdirectory(integration_test)
endif()
add_subdirectory(transform_example)
add_subdirectory(usr_msg_topic_example)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/segar_config/segar_setup.bash DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/segar_config/config DESTINATION ${CMAKE_INSTALL_PREFIX})
set(TRACING_NODE_COMPONENT_SO "${THIRD_PARTY_PREFIX}/lib/libtracing_node_component.so")
if(EXISTS "${TRACING_NODE_COMPONENT_SO}")
    install(FILES "${TRACING_NODE_COMPONENT_SO}"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/integration_test/lib)
else()
    message(WARNING "libtracing_node_component.so not found at ${TRACING_NODE_COMPONENT_SO}")
endif()
