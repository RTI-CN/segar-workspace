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

add_subdirectory(action_example)
add_subdirectory(component_example)
add_subdirectory(param_example)
add_subdirectory(service_example)
add_subdirectory(concurrent_example)
add_subdirectory(topic_example)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/segar_config/segar_setup.bash DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/segar_config/config DESTINATION ${CMAKE_INSTALL_PREFIX})
