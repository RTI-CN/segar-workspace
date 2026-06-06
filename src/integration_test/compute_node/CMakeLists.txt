set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有IDL相关文件


# NodeA 组件
add_library(node_a_component SHARED node_a.cc stress_cpu.cc)
target_link_libraries(node_a_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# SyncIntegrationNode 组件（基于 SyncComponent 的多路同步节点）
add_library(sync_integration_component SHARED sync_integration_node.cc stress_cpu.cc)
target_link_libraries(sync_integration_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# NodeB 组件
add_library(node_b_component SHARED node_b.cc stress_cpu.cc)
target_link_libraries(node_b_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeC 组件
add_library(node_c_component SHARED node_c.cc stress_cpu.cc)
target_link_libraries(node_c_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeD 组件
add_library(node_d_component SHARED node_d.cc stress_cpu.cc)
target_link_libraries(node_d_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeE 组件
add_library(node_e_component SHARED node_e.cc stress_cpu.cc)
target_link_libraries(node_e_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeF 组件
add_library(node_f_component SHARED node_f.cc stress_cpu.cc)
target_link_libraries(node_f_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeG 组件
add_library(node_g_component SHARED node_g.cc stress_cpu.cc)
target_link_libraries(node_g_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# NodeH 组件
add_library(node_h_component SHARED node_h.cc stress_cpu.cc)
target_link_libraries(node_h_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )
 

add_library(node_i_component SHARED node_i.cc stress_cpu.cc)
target_link_libraries(node_i_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# NodeJ 组件
add_library(node_j_component SHARED node_j.cc stress_cpu.cc)
target_link_libraries(node_j_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# NodeK 组件
add_library(node_k_component SHARED node_k.cc stress_cpu.cc)
target_link_libraries(node_k_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# NodeL 组件
add_library(node_l_component SHARED node_l.cc stress_cpu.cc)
target_link_libraries(node_l_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# NodeM 组件
add_library(node_m_component SHARED node_m.cc stress_cpu.cc)
target_link_libraries(node_m_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )

# ParamTimer 组件
add_library(param_timer_component SHARED param_timer_component.cc stress_cpu.cc)
target_link_libraries(param_timer_component rti::segar integration_test_msg examples_msg ${GENERATED_PROTO_OBJ_LIB} )


# 安装目标
install(TARGETS 
    node_a_component 
    sync_integration_component
    node_b_component 
    node_c_component 
    node_d_component 
    node_e_component 
    node_f_component 
    node_g_component 
    node_h_component
    node_i_component
    node_j_component
    node_k_component
    node_l_component
    node_m_component
    param_timer_component
    DESTINATION integration_test/lib)

# 安装DAG文件
install(DIRECTORY dag/
    DESTINATION integration_test/dag
    FILES_MATCHING PATTERN "*.dag")
