
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(segar_node_b segar_node_b.cc)
target_link_libraries(segar_node_b 
    rti::segar
    integration_test_msg
    )

add_executable(segar_node_a segar_node_a.cc)
target_link_libraries(segar_node_a 
    rti::segar
    integration_test_msg
    )

# 安装目标
install(TARGETS segar_node_b DESTINATION integration_test/bin)
install(TARGETS segar_node_a DESTINATION integration_test/bin)
