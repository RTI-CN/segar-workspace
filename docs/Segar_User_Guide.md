# ![Project Logo](./segar_logo.jpg)Segar User Guide

## 简介

- Segar 是追求极致可用性的机器人操作系统，它兼容主流软硬件生态，通过一体化工具链和极致的工程化能力，帮助团队快速构建业务流与数据闭环，以降低交付周期、运维成本和复杂度。
- Segar 集成了协程确定优先级调度、消息自动同步、内生告警接口、统一Qos管理、仿真时间、统一IO等特性，在吞吐、时延、抖动和资源占用上追求极致表现，提供 launch、monitor、recorder、tracing 等一体化工具链，并支持行业主流的三方生态工具。

## 系统概览

- [Segar History And Current State](Segar_History_And_Current_State.md) —— Segar的过去与现在

## 名词概念

- [Segar Terms](Segar_Terms.md) —— 常见术语速查

## 快速开始

- [Quick Start](Segar_Quick_Start.md) —— 第一个组件示例、构建与运行闭环
- [Segar Engineering](Segar_Engineering.md) —— 先了解 workspace 目录结构、文件放置位置、部署目录和运行方式

## 基本通信范式

- [Segar Topic](Segar_Topic.md) —— 发布订阅、Writer/Reader、`.msg` 类型
- [Segar Service](Segar_Service.md) —— 请求响应、Server/Client、`.srv` 类型
- [Segar Action](Segar_Action.md) —— 长时任务、ActionServer/ActionClient、`.action` 类型
- [Segar Parameter](Segar_Parameter.md) —— 本地/远程参数、参数配置与 API

## 编程框架

- [Segar Component](Segar_Component.md) —— 组件化开发、定时触发 Component、消息触发 Component
- [Segar Lifecycle](Segar_Lifecycle.md) —— Node / Component 的激活与非激活控制

## 任务调度框架

- [Segar Scheduler](Segar_Scheduler.md) —— 任务调度、协程、异步执行

## 并发与用户自定义 Task

- [Segar Concurrent And User-Defined Tasks](Segar_Concurrent_And_User_Defined_Tasks.md) —— `Async`、`CreateTask`、`WaitEvent`、`ReYield` 与协程兼容等待/唤醒

## 时间与定时器

- [Segar Time And Timer](Segar_Time_And_Timer.md) —— `Time`、`SleepUntil`、`MonoTime`、`Timer` 与周期任务

## 应用集成

- [Segar Launch](Segar_Launch.md) —— 使用 launch 文件管理与集成本地应用

## 工具链

- [Segar Tracing](Segar_Tracing.md) —— 追踪与诊断
- [SegarHeader Configuration](Segar_SegarHeader.md) —— 通过 `type_src/transport_messages` 配置自动添加头字段
- [仿真与数采支持](Segar_Sim_And_Data_Aacquisition.md) —— segar针对仿真和数采提供的接口
- [Segar CLI](Segar_Cli.md) —— `node` / `topic` / `service` / `action` / `param` / `bag` 子命令
- [Segar Bag](Segar_Recorder.md) —— bag 录制与回放
- [Segar ROS 2 Compatibility](Segar_Ros2_Compatible.md) —— ROS 2 生态兼容与桥接
- [Segar Log](Segar_Log.md) —— 日志宏、日志目录、环境变量与排查入口

## 使用示例说明

- [Segar Examples](Segar_Examples.md) —— topic、service、param、action、component、concurrent 等示例概览

## 集成测试

- [Segar Integration Test](Segar_Integration_Test.md) —— 集成测试模块构建、运行与结果说明

## 三方库适配

- [Segar Transform](Segar_Transform.md) —— TF 发布与查询、TransformBroadcaster/Buffer

## API 参考

- [Segar API Reference](Segar_Api_Reference.md) —— 按分类整理的 Segar API 列表

## 测试报告

- [功能测试报告](test_reports/Functional_Test_Report.md) —— Segar 功能测试报告
- [集成测试报告](test_reports/Integration_Test_Report.md) —— Segar 集成测试报告(cpu加压至80%)
- 与ROS2性能对比测试
  - [Topic性能对比](test_reports/Segar_vs_Ros2_Topic_Test.md) —— Segar vs Ros2 Topic 性能对比测试报告
  - [Service性能对比](test_reports/Segar_vs_Ros2_Service_Test.md) —— Segar vs Ros2 Service 性能对比测试报告
  - [Action性能对比](test_reports/Segar_vs_Ros2_Action_Test.md) —— Segar vs Ros2 Action 性能对比测试报告
