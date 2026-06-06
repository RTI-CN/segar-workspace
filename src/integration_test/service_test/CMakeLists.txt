
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(segar_service segar_service_server.cc)
target_link_libraries(segar_service 
    rti::segar
    integration_test_msg
    )

add_executable(segar_client segar_service_client.cc)
target_link_libraries(segar_client 
    rti::segar
    integration_test_msg
    )

add_executable(segar_client_async segar_service_client_async.cc)
target_link_libraries(segar_client_async 
    rti::segar
    integration_test_msg
    )

# 安装目标
install(TARGETS segar_client segar_client_async DESTINATION integration_test/bin)
install(TARGETS segar_service DESTINATION integration_test/bin)
