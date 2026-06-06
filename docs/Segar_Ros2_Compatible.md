# Segar ROS 2 Compatibility

> **说明**：本节介绍 Segar 与 ROS 2 生态的兼容工具，目前支持通过 Foxglove 等工具进行数据分析与可视化，后续会对ros2生态其他工具进行扩展支持

---

## Foxglove 桥接

### 使用步骤

1. 参考环境变量设置章节完成环境变量配置（`source segar_setup.bash`）
2. 运行以下命令，启动 `segar_foxglove_bridge` 桥接工具
3. 当看到 `Foxglove bridge is running` 时，表示程序启动成功
4. 启动 Foxglove 客户端，连接端口选择 **8765**，无需再做额外设置

### 启动命令

```bash
segar_foxglove_bridge
```

### 示例输出

```text
I0211 10:48:34.907411 4083683 foxglove_bridge_core.cc:73] [segar_foxglove_bridge]Foxglove bridge started on port 8765
I0211 10:48:34.907438 4083683 main.cc:103] [segar_foxglove_bridge]Foxglove bridge is running. Connect to ws://localhost:8765
I0211 10:48:34.907445 4083683 main.cc:104] [segar_foxglove_bridge]Press Ctrl+C to stop.
```

连接地址：`ws://localhost:8765`。按 Ctrl+C 可停止桥接服务。
