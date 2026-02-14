# Segar User Manual

## Introduction

- Segar is an extensible robotics operating system for cross-industry use. It is compatible with mainstream software and hardware ecosystems. Through an integrated toolchain and strong engineering capabilities, it helps teams quickly build business workflows and data feedback loops to reduce delivery cycles, operations costs, and complexity.
- Segar integrates features such as deterministic scheduling, automatic message synchronization, endogenous alarm interfaces, and unified QoS management. It pursues ultimate performance in throughput, latency, jitter, and resource usage. It provides integrated toolchains such as launch, monitor, recorder, and tracing, and supports mainstream third-party ecosystem tools.

## Basic Communication Paradigms

- [Getting Started with Topic](Segar_Topic.md) - Publish and subscribe, Writer/Reader, .msg type
- [Getting Started with Service](Segar_Service.md) - Request response, Server/Client, .srv type
- [Getting Started with Action](Segar_Action.md) - Long-term tasks, ActionServer/ActionClient, .Action type
- [Getting Started with Parameter](Segar_Parameter.md) - Local/remote parameters, parameter configuration and API

## Programming Framework

- [Getting Started with Component](Segar_Component.md) - Component development, timing-triggered Component, message-triggered Component

## Task Scheduling Framework

- [Getting Started with Scheduler](Segar_Scheduler.md) - Task scheduling, coroutines, asynchronous execution

## Toolchain

- [Getting Started with Segar CLI](Segar_Cli.md) —— node / Topic / service / Action / param / bag subcommand
- [Compatible with ROS 2 ecosystem tools](Segar_Ros2_Compatible.md) —— ROS 2 ecosystem compatibility and bridging
- [Bag recording tool](Segar_Recorder.md) —— bag recording and playback
- [Tracing Tool](Segar_Tracing.md) - Tracing and Diagnosis

## Examples

- [Examples](Segar_Examples.md) - Overview of examples such as Topic, service, param, Action, component, concurrent, etc.
- [Engineering and Deployment Instructions](Segar_Engineering.md) - Type definition, dependency management, link configuration, deployment and operation methods

## Application Integration
- [Integrate multiple local applications](Segar_Launch.md) - Use launch scripts to manage and integrate local applications

## API Reference

- [API Reference Manual](Segar_Api_Reference.md) - Segar API list organized by category

## Test Reports

- [Functional Test Report](test_reports/Functional_Test_Report.md) —— Segar Functional Test Report
- [Integration Test Report](test_reports/Integration_Test_Report.md) —— Segar integration test report (cpu pressure to 80%)
- Performance comparison test with ROS2
    - [Topic performance comparison](test_reports/Segar_vs_Ros2_Topic_Test.md) —— Segar vs Ros2 Topic performance comparison test report
    - [Service performance comparison](test_reports/Segar_vs_Ros2_Service_Test.md) —— Segar vs Ros2 Service performance comparison test report
    - [Action performance comparison](test_reports/Segar_vs_Ros2_Action_Test.md) —— Segar vs Ros2 Action performance comparison test report
