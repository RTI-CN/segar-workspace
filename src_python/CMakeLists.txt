# Install-only Python example modules for segar-workspace.

function(_install_python_example_resources TARGET_NAME EXAMPLE_NAME SUBDIR_NAME)
    if(EXISTS ${CMAKE_SOURCE_DIR}/src_python/${EXAMPLE_NAME}/${SUBDIR_NAME}/config)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/src_python/${EXAMPLE_NAME}/${SUBDIR_NAME}/config/
            DESTINATION src_python/${EXAMPLE_NAME}/${TARGET_NAME}/config
            FILES_MATCHING
            PATTERN "*.*"
        )
    endif()

    file(GLOB SCRIPT_FILES "${CMAKE_SOURCE_DIR}/src_python/${EXAMPLE_NAME}/${SUBDIR_NAME}/scripts/*.sh")
    if(SCRIPT_FILES)
        install(PROGRAMS ${SCRIPT_FILES}
            DESTINATION src_python/${EXAMPLE_NAME}/${TARGET_NAME}/scripts
        )
    endif()
endfunction()

function(add_python_example TARGET_NAME MAIN_SCRIPT)
    get_filename_component(SUBDIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
    get_filename_component(EXAMPLE_NAME ${PARENT_DIR} NAME)

    install(FILES ${CMAKE_CURRENT_LIST_DIR}/${MAIN_SCRIPT}
        DESTINATION src_python/${EXAMPLE_NAME}/${TARGET_NAME}/src
    )

    _install_python_example_resources(${TARGET_NAME} ${EXAMPLE_NAME} ${SUBDIR_NAME})
endfunction()

add_subdirectory(topic_example)
add_subdirectory(service_example)
add_subdirectory(action_example)
add_subdirectory(param_example)
