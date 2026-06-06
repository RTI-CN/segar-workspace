set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(segar_action_server segar_action_server.cc)
target_link_libraries(segar_action_server
    rti::segar
    integration_test_msg
    examples_msg
)

add_executable(segar_action_client_sync segar_action_client_sync.cc)
target_link_libraries(segar_action_client_sync
    rti::segar
    integration_test_msg
    examples_msg
)

add_executable(segar_action_client_async segar_action_client_async.cc)
target_link_libraries(segar_action_client_async
    rti::segar
    integration_test_msg
    examples_msg
)

# 安装目标
install(TARGETS
    segar_action_server
    segar_action_client_sync
    segar_action_client_async
    DESTINATION integration_test/bin
)
