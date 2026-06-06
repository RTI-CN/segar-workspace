
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(sensor_node sensor_node.cc)
target_link_libraries(sensor_node 
    rti::segar
    integration_test_msg
    )

# 安装目标
install(TARGETS sensor_node DESTINATION integration_test/bin)
install(FILES sensor_config.json DESTINATION integration_test/params)
install(PROGRAMS start_sensor.sh DESTINATION integration_test/params)
