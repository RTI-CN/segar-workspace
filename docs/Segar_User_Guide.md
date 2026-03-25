# Segar User Manual

## Introduction

- Segar is an open robotics operating system for the entire industry. It is compatible with mainstream software and hardware ecosystems. Through an integrated toolchain and extreme engineering capabilities, it helps teams quickly build business flow and closed-loop data workflows to reduce delivery cycles, operations costs, and complexity.
- Segar integrates features such as deterministic scheduling, automatic message synchronization, built-in alert interfaces, and unified QoS management. It pursues high performance in throughput, latency, jitter, and resource usage, provides integrated toolchains such as launch, monitor, recorder, and tracing, and supports mainstream third-party ecological tools in the industry.

## Basic communication paradigm

- [Getting started with Topic](Segar_Topic.md) - Publish and subscribe, Writer/Reader, .msg type
- [Getting Started with Service](Segar_Service.md) - Request response, Server/Client, .srv type
- [Getting started with Action](Segar_Action.md) - long-term tasks, ActionServer/ActionClient, .action type
- [SegarHeader automatic injection configuration](Segar_SegarHeader.md) - Automatically add header fields through `type_src/transport_messages` configuration
- [Getting started with Parameter](Segar_Parameter.md) - Local/remote parameters, parameter configuration and API

## Programming framework

- [Getting Started with Component](Segar_Component.md) - Component-based development, timing-triggered Component, message-triggered Component

## Task scheduling framework

- [Getting started with Scheduler](Segar_Scheduler.md) - Task scheduling, coroutines, asynchronous execution

## Toolchain

- [Getting started with Segar CLI](Segar_Cli.md) —— node / topic / service / action / param / bag subcommand
- [Compatible with ROS2 ecological tools](Segar_Ros2_Compatible.md) —— ROS 2 ecological compatibility and bridging
- [Broadcast bag recording tool] (Segar_Recorder.md) —— bag recording and playback
- [Tracing Tool](Segar_Tracing.md) - Tracing and Diagnosis

## Example description

- [Usage Examples](Segar_Examples.md) - Overview of examples such as topic, service, param, action, component, concurrent, etc.
- [Engineering and Deployment Instructions](Segar_Engineering.md) - Type definition, dependency management, link configuration, deployment and operation methods

## Application integration
- [Integrate multiple local applications](Segar_launch_tutorial.md) - Use launch scripts to manage and integrate local references

## API Reference

- [API Reference Manual](Segar_Api_Reference.md) - Segar API list organized by category

## Test report

- [Functional Test Report](test_reports/Functional_Test_Report.md) —— Segar functional test report
- [Integration Test Report](test_reports/Integration_Test_Report.md) —— Segar integration test report (cpu pressure to 80%)
- Performance comparison test with ROS2
    - [Topic performance comparison](test_reports/Segar_vs_Ros2_Topic_Test.md) —— Segar vs Ros2 Topic performance comparison test report
    - [Service performance comparison](test_reports/Segar_vs_Ros2_Service_Test.md) —— Segar vs Ros2 Service performance comparison test report
    - [Action performance comparison](test_reports/Segar_vs_Ros2_Action_Test.md) —— Segar vs Ros2 Action performance comparison test report
