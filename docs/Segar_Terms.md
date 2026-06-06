# Segar Terms

> 本页用于快速解释 Segar 中最常见的术语，便于阅读其它用户手册时快速对照。

| 术语 | 简要说明 |
|---|---|
| **`Node`** | Segar 的基础通信载体。Writer、Reader、Service、Client、Action、Parameter 等能力都依附在 Node 上。 |
| **`Component`** | Segar 中最常见的业务执行单元。通常通过 DAG 配置加载，核心入口是 `Init()` 和 `Proc()`。 |
| **`TimerComponent`** | 按固定周期触发 `Proc()` 的组件类型，适合周期任务、心跳和定时发布。 |
| **`SyncComponent`** | 多路消息按时间窗匹配成功后再触发 `Proc()` 的组件类型，适合多传感器同步或融合场景。 |
| `DAG file` | 组件启动配置文件，用于描述要加载哪些组件、依赖哪些 Topic，以及组件运行参数。 |
| `mainboard` | Segar 的组件启动器。业务库分布式开发，统一方式加载，通过 `mainboard -d <dag>` 加载一个或多个 DAG 并启动组件。 |
| `process_group` | 进程组名，用于选择 `config/<process_group>.conf` 这类调度配置文件。 |
| `Launch file` | 用于批量启动多个模块或进程的配置文件。适合管理整套应用的启动顺序。 |
| **`Topic`** | 发布订阅通信通道。发送方写入 Topic，接收方订阅同名 Topic。 |
| **`Reader / Writer`** | Topic 的读写接口。`Writer` 负责发布消息，`Reader` 负责接收消息。 |
| **`Service / Client`** | 请求响应通信模式。`Client` 发起请求，`Service` 处理请求并返回结果。 |
| **`Action`** | 面向长时任务的通信模式。支持发送 goal、接收 feedback、获取最终 result。 |
| **`Parameter`** | 参数访问接口。可用于读取或修改节点运行参数。 |
| **`Message`** | Segar 中传输的数据对象，可以来自 `.msg`、`.srv`、`.action` 或 `.proto` 定义。 |
| `transport_messages` | `type_src/transport_messages` 清单文件，用于声明哪些一级传输消息需要自动注入 `segar_header`。 |
| `segar.pb.conf` | 目标私有运行配置文件，常用于配置传输、调度默认项、运行模式等。 |
| `SEGAR_PATH` | 当前目标的本地运行根目录，程序通常从这里加载私有配置和参数文件。 |
| `SEGAR_GLOBAL_PATH` | `output/` 根目录，程序通常从这里加载全局配置，如 `topics.pb.conf`、`tracing_config.pb.txt`。 |
| `Proc()` | Node/Component 体系里的核心业务处理入口；普通 `Component`、`TimerComponent`、`SyncComponent` 的触发时机不同。 |
| **`Task`** | 调度器中的执行任务抽象。业务回调、组件处理逻辑等最终都会以任务形式参与调度。 |
| **`CRoutine`** | Segar 中的协程执行单元，用于提升线程利用率和任务调度效率。 |
| `Scheduler` | Segar 的任务调度器。负责线程池、优先级、CPU 亲和性和任务分配。 |
| **`Lifecycle`** | 运行态激活/非激活控制能力。可用于暂停或恢复 Node / Component 的业务处理。 |
| `Record file` | 录包得到的数据文件，用于回放历史 Topic 数据或复现问题现场。 |
| `Service discovery` | 节点发现 Service 的机制。Segar 不依赖中心节点，而是通过去中心化方式完成发现。 |

## 使用建议

- 第一次接触 Segar 时，建议先看 `Node`、`Component`、`DAG file`、`segar.pb.conf`、`SEGAR_PATH / SEGAR_GLOBAL_PATH`
- 阅读通信相关文档时，优先区分 `Topic`、`Service`、`Action`、`Parameter`
- 阅读组件和运行态控制文档时，重点区分 `Component`、`TimerComponent`、`SyncComponent`、`Proc()`、`Lifecycle`
- 阅读调优相关文档时，重点关注 `process_group`、`Task`、`CRoutine`、`Scheduler`

如需查看某个术语的完整使用方法，请继续阅读对应专题文档，例如 [Segar_Topic.md](Segar_Topic.md)、[Segar_Component.md](Segar_Component.md)、[Segar_Scheduler.md](Segar_Scheduler.md)。
