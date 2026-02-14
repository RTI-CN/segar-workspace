# Segar ROS 2 Ecological Compatibility Tool

> **Note**: This section introduces the compatible tools between Segar and the ROS 2 ecosystem. It currently supports data analysis and visualization through tools such as Foxglove, and will expand support for other tools in the ros2 ecosystem in the future.

---

## Foxglove bridge

### Usage steps

1. Refer to the environment variable setting chapter to complete the environment variable configuration (`source segar_setup.bash`)
2. Run the following command to start the `segar_foxglove_bridge` bridge tool
3. When you see `Foxglove bridge is running`, it means the program started successfully
4. Start the Foxglove client and select **8765** for the connection port. No additional settings are required.

### Start command

```bash
segar_foxglove_bridge
```

### Example output

```text
I0211 10:48:34.907411 4083683 foxglove_bridge_core.cc:73] [segar_foxglove_bridge]Foxglove bridge started on port 8765
I0211 10:48:34.907438 4083683 main.cc:103] [segar_foxglove_bridge]Foxglove bridge is running. Connect to ws://localhost:8765
I0211 10:48:34.907445 4083683 main.cc:104] [segar_foxglove_bridge]Press Ctrl+C to stop.
```

Connection address: `ws://localhost:8765`. Press Ctrl+C to stop the bridge service.
