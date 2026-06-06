# Segar Examples

文档包含编译说明、运行说明及各示例介绍。

---

## 编译环境说明

请在ubuntu系统上使用该仓库(推荐ubuntu 22.0.4)。
构建环境要求使用 CMake 3.10+、C++17。请确保已安装pip3（可以使用sudo apt install python3-pip安装）。
x86 与 Orin 使用不同编译器与脚本。

如需使用 Docker 统一编译/运行环境，见：[Docker 环境说明](Segar_Docker.md)。

### x86 编译

- **编译器**：GCC 9.5.0（x86_64-linux-gnu）
- **步骤**：在仓库根目录执行：

```bash
./scripts/build_x86.sh
```

- **可选参数**：`-d` 编译 Debug；`-r` 清理 build 目录；`-ra` 同时清理 build 与 `install/x86_64`；`-it` 启用 `integration_test` 模块构建
- **产物目录**：`build_x86/output/`
- **打包**：`./scripts/pkg_x86.sh`，将 `build_x86/output/` 打包为 tgz，存放在 `build_x86` 目录

### Orin 交叉编译

- **编译器**：GCC 11.4.0（`aarch64-linux-gnu`，用于 Orin/ARM64）
- **工具链**：使用 Ubuntu 22.04 系统包提供的交叉编译工具链
- **安装**：`sudo apt install gcc-11-aarch64-linux-gnu g++-11-aarch64-linux-gnu`
- **步骤**：在仓库根目录执行：

```bash
./scripts/build_orin.sh
```

- **可选参数**：`-d` 编译 Debug；`-r` 清理 build 目录；`-ra` 同时清理 build 与 `install/orin`；`-it` 启用 `integration_test` 模块构建
- **产物目录**：`build_orin/output/`
- **打包**：`./scripts/pkg_orin.sh`，将 `build_orin/output/` 打包为 tgz，存放在 `build_orin` 目录

### aarch64编译
目前orin交叉编译用的就是通用arm平台交叉编译工具，因为可直接复用orin相关的编译和打包命令

---

## 运行说明

### 示例运行

可执行文件位于各平台的 `output` 目录下，例如：`build_x86/output/<example_name>/<target_name>/` 或 `build_orin/output/...`。

- **单个示例**：进入对应示例目录后执行其 `scripts/launch.sh`：

```bash
cd build_x86/output/<example_name>/<target_name>
./scripts/launch.sh
```

`launch.sh` 会在启动时自动切换到当前模块根目录，因此既可以在模块根目录执行 `./scripts/launch.sh`，也可以直接在 `scripts/` 目录中执行 `bash launch.sh`。

- **一键启动所有示例**（后台）：在编译产物的 output 目录下执行，日志写在 `logs/` 下：

```bash
./scripts/start_all.sh   # 启动所有进程
./scripts/stop_all.sh    # 停止所有进程
./scripts/check_all.sh   # 查看所有进程状态
```

### segar CLI 运行

`segar` 命令行工具用于查看/操作节点、话题、服务、动作、参数、bag 等。需先启动示例或 `start_all.sh`，再在**同一 output 目录下**执行：

```bash
./scripts/run_segar_cli_test.sh
```

脚本会依次执行 `segar param`、`segar node`、`segar topic`、`segar service`、`segar action`、`segar bag` 等子命令，日志写在 `logs/` 下。

**运行单个 segar 命令**：在 output 目录下执行 `source segar_setup.bash` 导入环境变量后，可分别手动执行 `segar param` 等命令。

**运行 tracing**：在 output 目录下执行 `source segar_setup.bash` 导入环境变量后，执行 `mainboard -d config/tracing_node.dag` 开启 tracing 数据采集。首次使用需 `sudo tracing -i` 初始化 MySQL；导入与查询见 [Segar Tracing](Segar_Tracing.md)。

---

## 示例列表

### 1. topic_example - Topic 发布订阅示例

演示 Segar 框架中 Topic 的发布订阅功能：

- **topic_talker**：发布者示例，使用 `Timer` 周期性发布消息到 `/topic/chatter` 话题
- **topic_listener**：订阅者示例，订阅 `/topic/chatter` 话题并接收消息

**关键特性**：

- 使用 `CreateWriter` 创建发布者
- 使用 `CreateReader` 创建订阅者
- 使用 `Timer` 实现周期性发布

---

### 1.1 usr_msg_topic_example - 通用消息库 TypeCoverage Topic 示例

演示应用工程直接使用通用消息库 `usr_msg` 中的 `test_msgs/msg/TypeCoverage`，用于覆盖 Python/C++ topic 互通和常见内置字段类型：

- **type_coverage_talker**：发布 `test_msgs::msg::TypeCoverage` 到 `/topic/usr_msg/type_coverage`
- **type_coverage_listener**：订阅 `/topic/usr_msg/type_coverage` 并打印类型覆盖字段

**运行方式**：

```bash
cd build_x86/output/usr_msg_topic_example/type_coverage_listener
./scripts/launch.sh
```

再开一个终端：

```bash
cd build_x86/output/usr_msg_topic_example/type_coverage_talker
./scripts/launch.sh
```

Python 版本位于 `src_python/usr_msg_topic_example/`，见 [Segar Python 工程示例](../docs_python/Segar_Python_Examples.md)。

---

### 2. service_example - Service 服务调用示例

演示 Segar 框架中 Service 的请求响应功能：

- **service_server**：服务端示例，提供 `set_camera_info` 服务
- **service_client_sync**：同步客户端示例，使用同步方式调用服务
- **service_client_async**：异步客户端示例，使用异步方式调用服务

**关键特性**：

- 使用 `CreateService` 创建服务端
- 使用 `CreateClient` 创建客户端
- 同步调用：`SyncSendRequest`
- 异步调用：`SendRequest` 配合回调函数
- 使用 `Timer` 实现周期性请求

---

### 3. param_example - Parameter 参数管理示例

演示 Segar 框架中 Parameter 的参数管理功能：

- **param_server**：参数服务器示例，使用本地参数接口（Local Parameter API）
- **param_client**：参数客户端示例，使用远程参数接口（Remote Parameter API）

**关键特性**：

- 本地参数操作：`Segar_Set_Local_Param`、`Segar_Get_Local_Param`、`Segar_List_Local_Params`、`Segar_Dump_Local_Params`、`Segar_Load_Local_Params`
- 远程参数操作：`Segar_Get_Remote_Param`、`Segar_Set_Remote_Param`、`Segar_List_Remote_Params`、`Segar_Dump_Remote_Params`、`Segar_Load_Remote_Params`
- 支持基本类型（int、string 等）和 Protobuf 消息类型
- 使用自定义 proto 文件定义参数结构
- workspace 中示例使用的 protobuf 类型统一放在 `src/type_src/segar/proto/`，例如 `param_example.proto`

---

### 4. action_example - Action 动作执行示例

演示 Segar 框架中 Action 的长时间任务执行功能：

- **action_server**：动作服务器示例，执行 `lookup_transform` 动作
- **action_client_sync**：同步客户端示例，使用同步方式发送目标并等待结果
- **action_client_async**：异步客户端示例，使用异步方式发送目标，通过回调接收反馈和结果

**关键特性**：

- 使用 `CreateActionServer` 创建动作服务器
- 使用 `CreateActionClient` 创建动作客户端
- 支持目标（Goal）、反馈（Feedback）、结果（Result）的完整生命周期
- 支持目标取消（Cancel）功能
- 同步调用：`SyncSendGoal` + `WaitForResult`
- 异步调用：`AsyncSendGoal` + 回调函数
- 使用 `Timer` 实现周期性发送目标

---

### 5. component_example - Component 组件示例

演示 Segar 框架中 Component 的组件化开发方式：

- **timer_component**：定时器组件示例，按 `interval` 周期发布 `Image` 到 `/topic/image_front`（前视）、`/topic/image_rear`（后视）两路话题
- **common_component**：通用组件示例，同时接收三路消息：`/topic/chatter`（String）、`/topic/image_front`（Image）、`/topic/image_rear`（Image），由**消息触发**（首路有新消息时触发 Proc）
- **sync_component**：同步组件示例，**多路消息按时间窗同步**后触发 Proc，接收三路：`/topic/image_front`（REQUIRED）、`/topic/image_rear`（WAITABLE）、`/topic/chatter`（OPTIONAL）

**关键特性**：

- `TimerComponent` 继承自定时器组件，实现周期性发布任务
- `Component` 支持多消息类型订阅，通过模板参数指定消息类型，触发时机为“第一个 reader 有新消息”
- `SyncComponent` 支持多路消息时间对齐：每路可配置为 REQUIRED（必须匹配）、WAITABLE（实时必需、超时可缺）、OPTIONAL（可选），配合 `accept_diff`、`main_msg_cycle`、`accept_latency` 等参数做同步匹配与超时补偿
- 组件化开发，便于模块化管理

**运行方式**：

- 仅 **timer**：`component_example/timer_component/scripts/launch.sh`（发前视/后视两路）
- 仅 **common**：`component_example/common_component/scripts/launch.sh`（收三路，需其他节点发 chatter/image_front/image_rear）
- **timer + sync**（推荐联调）：`component_example/sync_component/scripts/launch.sh`，加载 `sync.dag`，同时起 timer（发前视/后视）与 sync_component（收三路，chatter 为 OPTIONAL 可缺）

**注意**：common_component / sync_component 若需 `/topic/chatter`，需另有节点（如 topic_talker）发布 String；仅跑 sync.dag 时 timer 只发两路 Image，chatter 可选。

---

### 6. concurrent_example - 并发基础设施示例

演示 Segar 框架中 `Async + WaitEvent` 在通信场景下的使用：

- **wait_event_talker**：先发送 `prepare`，再通过 `WaitEvent` 等待 `ready` 确认，收到后继续发送 `go`
- **wait_event_listener**：接收 `prepare` 后回复 `ready`，接收 `go` 后继续后续处理

**关键特性**：

- **Async**：启动后台任务，不阻塞主线程
- **WaitEvent**：在收到外部确认前挂起当前执行流
- **Topic 通信**：通过 `request/ack` 两个 Topic 完成跨进程握手
- **TRANSIENT_LOCAL**：保证首条握手消息在发现时序下仍可被后启动端接收

**示例场景**：

- `wait_event_talker` 通过 `Async` 启动握手流程
- `wait_event_talker` 发送 `prepare` 后调用 `WaitEvent::WaitFor()`
- `wait_event_listener` 收到 `prepare` 后发送 `ready`
- `wait_event_talker` 收到 `ready` 后调用 `WaitEvent::Notify()`，再继续发送 `go`

完整用法说明见 [Segar Concurrent And User-Defined Tasks](Segar_Concurrent_And_User_Defined_Tasks.md)。

---

### 7. zero_copy_example - Topic Zero Copy 示例

演示 Segar 框架中 Topic zero-copy 的基本使用方式：

- **zero_copy_talker**：发送端示例，使用 `CreateWriter<Image>` 和 `LoanSample()` 申请 zero-copy sample，并发布到 `/topic/zero_copy`
- **zero_copy_listener**：接收端示例，订阅 `/topic/zero_copy`，直接以 `std::shared_ptr<Image>` 读取消息

**关键特性**：

- 使用 `example::msg::Image` 作为 Topic 消息类型
- 使用 `CreateWriter<Image>` 创建 zero-copy Writer
- 使用 `LoanSample()` 获取 loaned sample
- 使用 `Write(sample)` 发送 zero-copy 消息
- 接收端直接通过 `std::shared_ptr<Image>` 访问消息字段

**说明**：

- 该示例直接复用已有的 `example/msg/Image.msg`
- 这表示 `LoanSample()` 是 Topic 的统一消息创建接口，不需要为 zero-copy 专门定义新的消息类型
- 当消息满足 Plain 标准或 `IsBoundedMessage<T>` 时，`LoanSample()` 会走 zero-copy 路径
- 当消息不满足 zero-copy 条件时，`LoanSample()` 会静默降级为普通消息分配

---

### 8. transform_example - segar-transform 坐标变换示例

演示 `segar-transform` 的 TF 发布与查询流程：

- **transform_broadcaster**：发布 `world -> base_link` 动态变换到 `rt/tf`
- **transform_listener**：订阅 TF 并查询 `base_link <- world` 动态变换结果
- **transform_static_broadcaster**：发布 `map -> camera_link` 静态变换到 `rt/tf_static`
- **transform_static_listener**：订阅 TF 并查询 `camera_link <- map` 静态变换结果

**关键特性**：

- 使用 `rti::segar::transform::TransformBroadcaster` 发布 `TransformStamped`
- 使用 `rti::segar::transform::Buffer` 执行 `canTransform` 与 `lookupTransform`
- 示例中可观察到平移量随时间变化，便于验证链路正确性

**运行方式**：

- `transform_example/transform_broadcaster/scripts/launch.sh`
- `transform_example/transform_listener/scripts/launch.sh`
- `transform_example/transform_static_broadcaster/scripts/launch.sh`
- `transform_example/transform_static_listener/scripts/launch.sh`

更多 API 和使用细节见：[Segar Transform](Segar_Transform.md)。

---

### 9. lifecycle_example - Lifecycle 激活/非激活示例

演示 Segar Lifecycle 在 Node 与 Component 下的基本使用方式：

- **node_lifecycle**：展示 `Node` 默认创建后处于 `Active`，以及调用 `Deactivate()` / `Activate()` 后对 Topic 收发行为的影响
- **component_lifecycle**：展示 `Component` 继承体系下同样的 `Activate()` / `Deactivate()` / `GetState()` 用法。当前示例以 `TimerComponent` 为例，展示非激活时业务 `Proc()` 不再执行

**关键特性**：

- `Node` 支持 `Activate()`、`Deactivate()`、`GetState()`、`IsLifecycleActive`
- `Component` / `TimerComponent` / `SyncComponent` 复用同一组生命周期接口
- `Inactive` 时，Topic / Service / Action 的业务处理会暂停；`Parameter` 不受影响
- 适合用于临时暂停业务、快速恢复运行、主备切换前的摘流等场景

**运行方式**：

- `lifecycle_example/node_lifecycle/scripts/launch.sh`
- `lifecycle_example/component_lifecycle/scripts/launch.sh`

每个示例目录包含：**源码** `src/`、**配置** `config/`、**启动脚本** `scripts/launch.sh`。

---

### 9. integration_test - 集成测试模块

`integration_test` 用于验证 Topic/Service/Action/参数定时器在复杂拓扑下的稳定性、时延和资源占用表现。

**构建方式**：

```bash
./scripts/build_x86.sh -it
# 或
./scripts/build_orin.sh -it
```

**产物目录**：

- `build_x86/output/integration_test/`
- `build_orin/output/integration_test/`

**运行方式**：

```bash
cd build_x86/output/integration_test
./run_integration_test.sh
./run_performance_test.sh
```

详细说明见 [Integration Test 使用说明](Segar_Integration_Test.md)。
