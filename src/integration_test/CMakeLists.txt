cmake_minimum_required(VERSION 3.10)
project(integration_test)

# Set up directories
set(INTEGRATION_TEST_GENERATE_DIR "${CMAKE_BINARY_DIR}/generate/integration_test")
set(INTEGRATION_TEST_IDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/type")

MsgToolCompile(${INTEGRATION_TEST_IDL_DIR} ${INTEGRATION_TEST_GENERATE_DIR} integration_test_msg)
_ensure_generated_namespace_package("${INTEGRATION_TEST_GENERATE_DIR}/src/python" "segar")

add_subdirectory(sensor_node)
add_subdirectory(compute_node)
add_subdirectory(topic_test)
add_subdirectory(service_test)
add_subdirectory(action_test)

install(DIRECTORY params/
    DESTINATION integration_test/params)

install(PROGRAMS tools/run_integration_test.sh tools/run_performance_test.sh tools/parse_itest_logs.py tools/check_perf_logs.py
    DESTINATION integration_test)

# tracing_node.dag loads the component from ../lib at runtime.
set(TRACING_NODE_COMPONENT_SO "${THIRD_PARTY_PREFIX}/lib/libtracing_node_component.so")
if(EXISTS "${TRACING_NODE_COMPONENT_SO}")
    install(FILES "${TRACING_NODE_COMPONENT_SO}"
        DESTINATION lib)
else()
    message(WARNING "libtracing_node_component.so not found at ${TRACING_NODE_COMPONENT_SO}")
endif()
