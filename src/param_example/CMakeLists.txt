# Find Protobuf

add_library(param_example_proto OBJECT proto/param_example.pb.cc)
target_include_directories(param_example_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/proto)
target_link_libraries(param_example_proto PUBLIC protobuf::libprotobuf)
add_subdirectory(param_server)
add_subdirectory(param_client)
