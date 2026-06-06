# Segar API Reference

## 简介

本文档适用于 C++ 开发者快速查阅 Segar 框架的常用接口。每个 API 包含：**简介**、**所属**（命名空间/类）、**命名空间**、**头文件**、**签名**、**参数**、**返回值**，以表格形式展示。

---

## 1. 基础与初始化

程序入口必需的初始化、节点创建与阻塞等待。**头文件**：`segar/segar.h`

### Init

| 字段 | 说明 |
|------|------|
| 简介 | 程序入口必须调用的 Segar 运行时初始化函数 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool Init(const char* argv0)` |
| 参数 | `argv0`：程序入口参数，通常传 `argv[0]`，用于初始化 Segar 运行时 |
| 返回值 | `true` 表示初始化成功，`false` 表示失败。程序入口必须调用，失败时应退出 |

### CreateNode

| 字段 | 说明 |
|------|------|
| 简介 | 创建 Segar 节点，是创建 Writer/Reader/Service/Client/ActionServer/ActionClient 的载体 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<Node> CreateNode(const std::string& name)` |
| 参数 | `name`：节点名称，在同一进程中需唯一 |
| 返回值 | 节点智能指针，失败时返回空指针。新创建的 Node 默认处于 `Active` 状态 |

### Activate（Node）

| 字段 | 说明 |
|------|------|
| 简介 | 将 Node 从 `Inactive` 切换到 `Active`，恢复 Topic / Service / Action 的业务处理 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool Activate()` |
| 参数 | 无 |
| 返回值 | `true` 表示切换成功；若当前已是 `Active`，返回 `false` |

### Deactivate（Node）

| 字段 | 说明 |
|------|------|
| 简介 | 将 Node 从 `Active` 切换到 `Inactive`，暂停 Topic / Service / Action 的业务处理 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool Deactivate()` |
| 参数 | 无 |
| 返回值 | `true` 表示切换成功；若当前已是 `Inactive`，返回 `false` |

### GetState（Node）

| 字段 | 说明 |
|------|------|
| 简介 | 获取 Node 当前生命周期状态 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `LifecycleState GetState() const` |
| 参数 | 无 |
| 返回值 | 返回 `Node::LifecycleState::Active` 或 `Node::LifecycleState::Inactive` |

### IsLifecycleActive（Node）

| 字段 | 说明 |
|------|------|
| 简介 | 判断 Node 当前是否处于 `Active` 状态 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool IsLifecycleActive() const` |
| 参数 | 无 |
| 返回值 | `true` 表示当前为 `Active`；`false` 表示当前为 `Inactive` |

### WaitForShutdown

| 字段 | 说明 |
|------|------|
| 简介 | 阻塞当前线程直到收到退出信号（如 Ctrl+C），常用于保持主线程存活；可选传入回调函数用于退出前执行用户清理逻辑 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void WaitForShutdown(const std::function<void()>& shutdown_callback = nullptr)` |
| 参数 | `shutdown_callback`：（可选）收到退出信号后执行的回调，通常用于设置退出标志、join 用户线程、释放用户持有的 Reader/Writer/Node 等资源 |
| 返回值 | 无 |

---

## 2. Topic 发布订阅

基于 Topic 的发布/订阅通信，支持 Writer 发布、Reader 订阅回调。

### CreateWriter

| 字段 | 说明 |
|------|------|
| 简介 | 创建 Topic 发布者，用于向指定 Topic 发送消息 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<Writer<T>> CreateWriter<T>(const std::string& topic_name)` |
| 参数 | `topic_name`：Topic 名称，需与订阅端完全一致，如 `"/topic/chatter"`；`T`：模板参数，消息类型（来自 .msg 定义） |
| 返回值 | Writer 智能指针，失败时返回空指针 |

### Write

| 字段 | 说明 |
|------|------|
| 简介 | 向 Topic 发布一条消息 |
| 所属 | `Writer<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool Write(const std::shared_ptr<T>& msg)` |
| 参数 | `msg`：要发布的消息，类型为 `std::shared_ptr<T>` |
| 返回值 | `true` 表示发送成功，`false` 表示失败 |

### CreateReader

| 字段 | 说明 |
|------|------|
| 简介 | 创建 Topic 订阅者，通过回调接收消息 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<Reader<T>> CreateReader<T>(const std::string& topic_name, Callback callback[, options])` |
| 参数 | `topic_name`：Topic 名称，需与发布端完全一致；`callback`：收到消息时的回调；`options`：（可选）如 `pending_queue_size`（默认 5）、qos 等 |
| 返回值 | Reader 智能指针，失败时返回空指针 |

---

## 3. Service 服务调用

典型的请求-响应模式，Service 端处理请求并返回 Response，Client 端同步或异步调用。

### CreateService

| 字段 | 说明 |
|------|------|
| 简介 | 创建服务端，注册请求处理回调以响应客户端请求 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<Service<T>> CreateService<T>(const std::string& service_name, ServiceCallback callback)` |
| 参数 | `service_name`：服务名称，需与客户端完全一致；`callback`：处理请求的回调，需在回调内填充 response；`T`：服务类型（来自 .srv 定义） |
| 返回值 | Service 智能指针，失败时返回空指针 |

### CreateClient

| 字段 | 说明 |
|------|------|
| 简介 | 创建客户端，用于向服务端发起请求 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<Client<T>> CreateClient<T>(const std::string& service_name)` |
| 参数 | `service_name`：服务名称，需与服务端完全一致；`T`：服务类型 |
| 返回值 | Client 智能指针，失败时返回空指针 |

### SyncSendRequest

| 字段 | 说明 |
|------|------|
| 简介 | 同步发送服务请求并等待响应 |
| 所属 | `Client<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<T::Response> SyncSendRequest(const std::shared_ptr<T::Request>& request)` |
| 参数 | `request`：请求对象 |
| 返回值 | Response 智能指针，成功时非空，失败或超时时返回空指针 |

### AsyncSendRequest

| 字段 | 说明 |
|------|------|
| 简介 | 异步发送服务请求，通过回调接收响应 |
| 所属 | `Client<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void AsyncSendRequest(const std::shared_ptr<T::Request>& request, ResponseCallback callback)` |
| 参数 | `request`：请求对象；`callback`：签名为 `void(const std::shared_ptr<T::Response>& response)`，response 为空表示请求失败 |
| 返回值 | 无 |

---

## 4. Action 动作执行

长时间运行的动作（Goal/Feedback/Result），支持取消、进度反馈，Client 支持同步与异步调用。

### CreateActionServer

| 字段 | 说明 |
|------|------|
| 简介 | 创建 Action 服务端，处理 goal 接收、取消与执行 |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<ActionServer<T>> CreateActionServer<T>(const std::string& action_name, const ActionServer<T>::Callbacks& callbacks)` |
| 参数 | `action_name`：Action 名称；`callbacks`：包含 `on_goal`、`on_cancel`、`on_execute` 三个回调；`T`：Action 类型（来自 .action 定义） |
| 返回值 | ActionServer 智能指针，失败时返回空指针 |

### PublishFeedback

| 字段 | 说明 |
|------|------|
| 简介 | 向客户端发送 Action 执行进度反馈 |
| 所属 | `ActionServer<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void PublishFeedback(const GoalID& goal_id, const std::shared_ptr<T::Feedback>& feedback)` |
| 参数 | `goal_id`：当前 goal 的唯一标识；`feedback`：反馈消息 |
| 返回值 | 无 |

### CancelGoal

| 字段 | 说明 |
|------|------|
| 简介 | 取消指定 goal 并以 result 结束 |
| 所属 | `ActionServer<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void CancelGoal(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| 参数 | `goal_id`：要取消的 goal 标识；`result`：取消时的结果（可含部分数据） |
| 返回值 | 无 |

### Succeed

| 字段 | 说明 |
|------|------|
| 简介 | 标记 goal 成功完成并返回结果 |
| 所属 | `ActionServer<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void Succeed(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| 参数 | `goal_id`：已完成的 goal 标识；`result`：最终结果 |
| 返回值 | 无 |

### CreateActionClient

| 字段 | 说明 |
|------|------|
| 简介 | 创建 Action 客户端，用于发送 goal 并接收 feedback/result |
| 所属 | `Node` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::shared_ptr<ActionClient<T>> CreateActionClient<T>(const std::string& action_name[, const GoalCallbacks& callbacks])` |
| 参数 | `action_name`：Action 名称；`callbacks`：（可选）GoalCallbacks，包含 `on_accept`、`on_result`、`on_cancel`、`on_feedback` |
| 返回值 | ActionClient 智能指针，失败时返回空指针 |

### SyncSendGoal

| 字段 | 说明 |
|------|------|
| 简介 | 同步发送 goal，阻塞直到服务端接受或失败 |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool SyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| 参数 | `goal`：要发送的 goal 对象；`goal_id`：输出参数，用于接收该 goal 的唯一标识 |
| 返回值 | `true` 表示成功发送且服务端已接受，`false` 表示失败 |

### SyncCancelGoal

| 字段 | 说明 |
|------|------|
| 简介 | 同步发送取消请求并等待处理结果 |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool SyncCancelGoal(const GoalID& goal_id)` |
| 参数 | `goal_id`：要取消的 goal 标识 |
| 返回值 | `true` 表示取消请求已发送并得到处理，`false` 表示失败 |

### WaitForResult（重载 1）

| 字段 | 说明 |
|------|------|
| 简介 | 等待指定 goal 的 result |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool WaitForResult(const GoalID& goal_id)` |
| 参数 | `goal_id`：等待的 goal 标识 |
| 返回值 | `true` 表示在超时前收到 result，`false` 表示超时 |

### WaitForResult（重载 2）

| 字段 | 说明 |
|------|------|
| 简介 | 等待指定 goal 的 result，并获取 Result 和状态码 |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool WaitForResult(const GoalID& goal_id, T::Result* result, GoalStatusCode* status)` |
| 参数 | `goal_id`：等待的 goal 标识；`result`：输出参数，接收 Result；`status`：输出参数，接收状态码（如 STATUS_SUCCEEDED、STATUS_CANCELED、STATUS_ABORTED） |
| 返回值 | `true` 表示在超时前收到 result，`false` 表示超时 |

### AsyncSendGoal

| 字段 | 说明 |
|------|------|
| 简介 | 异步发送 goal，不等待服务端接受 |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `bool AsyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| 参数 | `goal`：要发送的 goal 对象；`goal_id`：输出参数，接收 goal 标识 |
| 返回值 | `true` 表示成功发送，`false` 表示发送失败（如无法连接服务端） |

### AsyncCancelGoal

| 字段 | 说明 |
|------|------|
| 简介 | 异步发送取消请求，不等待处理完成 |
| 所属 | `ActionClient<T>` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void AsyncCancelGoal(const GoalID& goal_id)` |
| 参数 | `goal_id`：要取消的 goal 标识 |
| 返回值 | 无 |

---

## 5. Parameter 参数

节点级参数管理：本地加载/设置/获取/导出，以及远程节点参数的获取与加载。

### Segar_Load_Local_Params

| 字段 | 说明 |
|------|------|
| 简介 | 从 YAML 或 dump 文件加载本地参数到节点 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h`（需配合 `segar/segar.h` 使用） |
| 签名 | `bool Segar_Load_Local_Params(Node* node, const std::string& path)` |
| 参数 | `node`：节点指针；`path`：YAML 或 dump 文件路径 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_List_Local_Params

| 字段 | 说明 |
|------|------|
| 简介 | 列出节点的所有本地参数 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_List_Local_Params(Node* node, std::vector<Parameter>* out_list)` |
| 参数 | `node`：节点指针；`out_list`：输出参数，接收参数列表 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Set_Local_Param

| 字段 | 说明 |
|------|------|
| 简介 | 设置节点的本地参数（Value 支持 int、string、Protobuf 等） |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Set_Local_Param(Node* node, const std::string& name, const Value& value)` |
| 参数 | `node`：节点指针；`name`：参数名；`value`：参数值 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Get_Local_Param

| 字段 | 说明 |
|------|------|
| 简介 | 获取节点的本地参数值 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Get_Local_Param(Node* node, const std::string& name, Value* out_value)` |
| 参数 | `node`：节点指针；`name`：参数名；`out_value`：输出参数，接收参数值 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Dump_Local_Params

| 字段 | 说明 |
|------|------|
| 简介 | 将节点的本地参数导出到文件 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Dump_Local_Params(Node* node, const std::string& path)` |
| 参数 | `node`：节点指针；`path`：保存文件路径 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Get_Remote_Param

| 字段 | 说明 |
|------|------|
| 简介 | 获取远程节点的参数值 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Get_Remote_Param(const std::string& node_name, const std::string& name, Value* out_value)` |
| 参数 | `node_name`：远程节点名称；`name`：参数名；`out_value`：输出参数 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Load_Remote_Params

| 字段 | 说明 |
|------|------|
| 简介 | 从文件加载参数到远程节点 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Load_Remote_Params(const std::string& node_name, const std::string& path)` |
| 参数 | `node_name`：远程节点名称；`path`：YAML 或 dump 文件路径 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_List_Remote_Params

| 字段 | 说明 |
|------|------|
| 简介 | 列出远程节点的所有参数 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_List_Remote_Params(const std::string& node_name, std::vector<Parameter>* out_list)` |
| 参数 | `node_name`：远程节点名称；`out_list`：输出参数 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

### Segar_Dump_Remote_Params

| 字段 | 说明 |
|------|------|
| 简介 | 将远程节点的参数导出到文件 |
| 所属 | C 接口 |
| 命名空间 | 无（全局 C 风格函数） |
| 头文件 | `segar/parameter/segar_parameter_api.h` |
| 签名 | `bool Segar_Dump_Remote_Params(const std::string& node_name, const std::string& path)` |
| 参数 | `node_name`：远程节点名称；`path`：保存文件路径 |
| 返回值 | `true` 表示成功，`false` 表示失败 |

---

## 6. Timer 定时器

基于毫秒间隔的一次性或周期性定时器，可指定回调与 `oneshot` 模式。

### Timer 构造函数

| 字段 | 说明 |
|------|------|
| 简介 | 创建一次性或周期性定时器，按指定间隔回调 |
| 所属 | `rti::segar::Timer` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `Timer(uint32_t period, std::function<void()> callback, bool oneshot)` |
| 参数 | `period`：执行间隔，单位毫秒；`callback`：无参回调，到期时执行；`oneshot`：`true` 表示只执行一次，`false` 表示周期执行 |
| 返回值 | 无（构造函数） |

### Start

| 字段 | 说明 |
|------|------|
| 简介 | 启动定时器，开始按间隔周期性调用 callback |
| 所属 | `rti::segar::Timer` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `void Start()` |
| 参数 | 无 |
| 返回值 | 无 |

---

## 7. 并发基础设施

异步执行、协程同步（WaitEvent）、锁、`ReYield()` 与睡眠，用于多任务与线程安全场景。

### Async

| 字段 | 说明 |
|------|------|
| 简介 | 异步执行可调用对象，返回 future 以便等待结果 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/segar.h` |
| 签名 | `std::future<R> Async(Callable&& callable)`（R 为 callable 返回类型） |
| 参数 | `callable`：可调用对象（函数、lambda 等） |
| 返回值 | `std::future`，可用于 `wait()`、`get()` 等待结果 |

### Scheduler::CreateTask

| 字段 | 说明 |
|------|------|
| 简介 | 创建一个独立命名的 CRoutine task，并交给 scheduler 调度 |
| 所属 | `rti::segar::scheduler::Scheduler` 单例 |
| 命名空间 | `rti::segar::scheduler` |
| 头文件 | `segar/scheduler/scheduler.h` |
| 签名 | `bool CreateTask(std::function<void()>&& func, const std::string& name, std::shared_ptr<DataVisitorBase> visitor = nullptr)` |
| 参数 | `func`：task 主体；`name`：task 名字；`visitor`：可选通知源 |
| 返回值 | `true` 表示创建成功，`false` 表示创建失败 |
| 注意 | 适合长驻任务、IO 协程和需要独立命名的任务；如果只是提交一次性短任务，更适合 `Async(...)` |

### Scheduler::NotifyTask

| 字段 | 说明 |
|------|------|
| 简介 | 按 task id 通知一个已创建 task 进入调度 |
| 所属 | `rti::segar::scheduler::Scheduler` 单例 |
| 命名空间 | `rti::segar::scheduler` |
| 头文件 | `segar/scheduler/scheduler.h` |
| 签名 | `bool NotifyTask(uint64_t crid)` |
| 参数 | `crid`：task id |
| 返回值 | `true` 表示通知已发送，`false` 表示通知失败 |
| 注意 | 这是偏底层的 task 唤醒接口。普通用户更推荐通过 `WaitEvent`、`SleepUntil`、`USleep`、`SleepFor` 等已封装方式控制 task 的等待与恢复 |

### Scheduler::RemoveTask

| 字段 | 说明 |
|------|------|
| 简介 | 按名字移除一个已创建 task |
| 所属 | `rti::segar::scheduler::Scheduler` 单例 |
| 命名空间 | `rti::segar::scheduler` |
| 头文件 | `segar/scheduler/scheduler.h` |
| 签名 | `bool RemoveTask(const std::string& name)` |
| 参数 | `name`：task 名字 |
| 返回值 | `true` 表示移除成功，`false` 表示未找到或移除失败 |
| 注意 | `CreateTask(...)` 创建的命名 task 在函数返回后不会自动从 scheduler 管理结构中注销；如果不再需要，通常应主动 `RemoveTask(name)` |

### WaitEvent

| 字段 | 说明 |
|------|------|
| 简介 | 进程内协程安全的通知事件 |
| 所属 | `rti::segar::WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | 类类型 |
| 参数 | — |
| 返回值 | — |

#### 成员操作

##### Notify

| 字段 | 说明 |
|------|------|
| 简介 | 非阻塞地发送一次通知；若当前无人等待，后续等待可以消费该通知 |
| 所属 | `WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `void Notify()` |
| 参数 | 无 |
| 返回值 | 无 |

##### NotifyAll

| 字段 | 说明 |
|------|------|
| 简介 | 广播通知当前所有等待者 |
| 所属 | `WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `void NotifyAll()` |
| 参数 | 无 |
| 返回值 | 无 |

##### Wait

| 字段 | 说明 |
|------|------|
| 简介 | 无限等待一次通知；在协程中则挂起当前协程，在普通线程中则退化为线程等待 |
| 所属 | `WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `bool Wait()` |
| 参数 | 无 |
| 返回值 | `true` 表示收到并消费了一次通知 |

##### WaitFor

| 字段 | 说明 |
|------|------|
| 简介 | 在给定超时时间内等待一次通知；在协程中则挂起当前协程，在普通线程中则退化为线程等待 |
| 所属 | `WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `bool WaitFor(std::chrono::milliseconds timeout, std::function<bool()> predicate)` |
| 参数 | `timeout`：超时时间；`predicate`：（可选）返回 `true` 时表示等待完成；为空时表示只等待一次通知 |
| 返回值 | `true` 表示条件在超时前成立，`false` 表示超时或等待被重置 |

##### Reset

| 字段 | 说明 |
|------|------|
| 简介 | 清空所有未消费的通知，并使当前这一轮正在等待的 Wait/WaitFor 立即返回 false |
| 所属 | `WaitEvent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `void Reset()` |
| 参数 | 无 |
| 返回值 | 无 |

### LockGuard

| 字段 | 说明 |
|------|------|
| 简介 | RAII 风格互斥锁，构造时加锁、析构时解锁，协程安全 |
| 所属 | `rti::segar::LockGuard<Mutex>` 模板类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `LockGuard(Mutex& mutex)` |
| 参数 | `mutex`：互斥量引用（如 `std::mutex`） |
| 返回值 | 无 |

### ReYield

| 字段 | 说明 |
|------|------|
| 简介 | 让出当前协程执行权，并以 `READY` 状态重新进入调度；适合短轮询和分步推进 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `void ReYield()` |
| 参数 | 无 |
| 返回值 | 无 |
| 注意 | 在非 CRoutine 模式下，当前实现会打印 warning，并退化为 `std::this_thread::yield()` |

### SleepFor

| 字段 | 说明 |
|------|------|
| 简介 | 协程安全睡眠，睡眠期间会让出执行权 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/task/task.h` |
| 签名 | `void SleepFor(std::chrono::duration d)` |
| 参数 | `d`：睡眠时长 |
| 返回值 | 无 |

### USleep

| 字段 | 说明 |
|------|------|
| 简介 | 以微秒为单位的固定时长睡眠；在协程中只挂起当前协程，在普通线程中退化为线程睡眠 |
| 所属 | `rti::segar` 命名空间 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/time/time.h` |
| 签名 | `void USleep(useconds_t usec)` |
| 参数 | `usec`：睡眠时长，单位为微秒 |
| 返回值 | 无 |
| 注意 | `USleep(1000000)` 表示睡 `1 s`；如果要睡到某个绝对时间点，请使用 `Time::SleepUntil(...)` |

### Time::MonoTime

| 字段 | 说明 |
|------|------|
| 简介 | 获取单调时钟时间戳，适合做耗时统计、间隔和超时计算 |
| 所属 | `rti::segar::Time` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/time/time.h` |
| 签名 | `static Time MonoTime()` |
| 参数 | 无 |
| 返回值 | 当前单调时钟时间 |
| 注意 | 单调时钟更适合做相对时间计算，不适合直接用于展示日期时间 |

### Time::SleepUntil

| 字段 | 说明 |
|------|------|
| 简介 | 睡到指定绝对时间点；在协程中只挂起当前协程，在普通线程中退化为线程睡眠 |
| 所属 | `rti::segar::Time` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/time/time.h` |
| 签名 | `static void SleepUntil(const Time& time)` |
| 参数 | `time`：目标时间点 |
| 返回值 | 无 |
| 注意 | 按当前实现，传入目标时间更适合基于 `Time::Now()` 计算；不建议直接把 `MonoTime()` 结果作为 `SleepUntil()` 参数 |

---

## 8. Component 组件

基于 DAG 的组件框架：事件触发（Component）、定时器触发（TimerComponent）与多路消息同步触发（SyncComponent），需配合 `SEGAR_REGISTER_COMPONENT` 注册。

### Component 基类

| 字段 | 说明 |
|------|------|
| 简介 | 事件触发组件基类，模板参数为输入消息类型，与 DAG 中 readers 顺序一致 |
| 所属 | `rti::segar::Component<InputType1, InputType2, ...>` 模板类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` |
| 签名 | 模板类 |
| 参数 | — |
| 返回值 | — |

### TimerComponent 基类

| 字段 | 说明 |
|------|------|
| 简介 | 定时器触发组件基类，按 DAG 中 interval 周期性调用 Proc |
| 所属 | `rti::segar::TimerComponent` 类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/timer_component.h` |
| 签名 | 类类型 |
| 参数 | — |
| 返回值 | — |

### SyncComponent 基类

| 字段 | 说明 |
|------|------|
| 简介 | 多 Topic 同步组件基类，按时间戳对多路输入做对齐/融合后触发 Proc |
| 所属 | `rti::segar::SyncComponent<M0, M1, ...>` 模板类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/sync_component.h` |
| 签名 | 模板类 |
| 参数 | — |
| 返回值 | — |

### Init（虚函数）

| 字段 | 说明 |
|------|------|
| 简介 | 组件初始化入口，返回 false 时组件将不会启动 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `virtual bool Init()` |
| 参数 | 无 |
| 返回值 | `true` 表示初始化成功，`false` 表示失败 |

### Activate（组件）

| 字段 | 说明 |
|------|------|
| 简介 | 将 `Component` / `TimerComponent` / `SyncComponent` 切换到 `Active`，恢复业务 `Proc()` 执行 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `bool Activate()` |
| 参数 | 无 |
| 返回值 | `true` 表示切换成功；若当前已是 `Active`，返回 `false` |

### Deactivate（组件）

| 字段 | 说明 |
|------|------|
| 简介 | 将 `Component` / `TimerComponent` / `SyncComponent` 切换到 `Inactive`，暂停业务 `Proc()` 执行 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `bool Deactivate()` |
| 参数 | 无 |
| 返回值 | `true` 表示切换成功；若当前已是 `Inactive`，返回 `false` |

### GetState（组件）

| 字段 | 说明 |
|------|------|
| 简介 | 获取 `Component` / `TimerComponent` / `SyncComponent` 当前生命周期状态 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `Node::LifecycleState GetState() const` |
| 参数 | 无 |
| 返回值 | 返回 `Node::LifecycleState::Active` 或 `Node::LifecycleState::Inactive` |

### IsLifecycleActive（组件）

| 字段 | 说明 |
|------|------|
| 简介 | 判断组件当前是否处于 `Active` 状态，通常用于组件内部或派生类辅助判断业务是否应继续执行 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `bool IsLifecycleActive() const` |
| 参数 | 无 |
| 返回值 | `true` 表示当前为 `Active`；`false` 表示当前为 `Inactive` |
| 访问说明 | 该接口为 `protected`，主要供组件基类内部或用户自定义组件派生类使用 |

### Proc（虚函数）

| 字段 | 说明 |
|------|------|
| 简介 | 组件业务逻辑入口，Component 按事件触发、TimerComponent 按 interval 触发、SyncComponent 按同步结果触发 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | `Component`：`virtual bool Proc(const std::shared_ptr<Input1>& msg1, ...)`；`TimerComponent`：`virtual bool Proc()`；`SyncComponent`：`virtual bool Proc(const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| 参数 | Component/SyncComponent 的 Proc 接收与 readers 对应的消息；TimerComponent 无参 |
| 返回值 | `true` 表示处理成功，`false` 表示失败（框架会记录错误） |

### GetTimeStamp（虚函数，SyncComponent）

| 字段 | 说明 |
|------|------|
| 简介 | 提供消息时间戳给同步器用于对齐与超时计算；需由 SyncComponent 子类实现 |
| 所属 | `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/sync_component.h` |
| 签名 | 单输入：`virtual uint64_t GetTimeStamp(const std::shared_ptr<M0>& msg0)`；多输入：`virtual uint64_t GetTimeStamp(size_t index, const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| 参数 | `index`：消息索引（多输入时）；`msg*`：各输入消息 |
| 返回值 | 时间戳（通常为消息时间，单位由业务定义，常见为纳秒） |

### TimeoutProc（可选虚函数，SyncComponent）

| 字段 | 说明 |
|------|------|
| 简介 | 同步周期内未满足处理条件或发生超时时触发，默认空实现，可用于降级/告警逻辑 |
| 所属 | `SyncComponent` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/sync_component.h` |
| 签名 | `virtual void TimeoutProc()` |
| 参数 | 无 |
| 返回值 | 无 |

### Fusionable（可选虚函数，SyncComponent 多输入）

| 字段 | 说明 |
|------|------|
| 简介 | 处理前的融合可行性判断钩子，返回 `false` 时当前组消息不会进入 Proc |
| 所属 | `SyncComponent<M0, M1, ...>` 基类 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/sync_component.h` |
| 签名 | `virtual bool Fusionable(const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| 参数 | 与 Proc 对齐的多路输入消息 |
| 返回值 | `true` 表示可处理；`false` 表示跳过该次融合结果 |

### CreateWriter（组件内）

| 字段 | 说明 |
|------|------|
| 简介 | 组件内通过 `node_->CreateWriter<T>(topic)` 创建发布者 |
| 所属 | `Component` / `TimerComponent` / `SyncComponent` 基类提供的 `node_` 成员 |
| 命名空间 | `rti::segar` |
| 头文件 | `segar/component/component.h` 或 `segar/component/timer_component.h` 或 `segar/component/sync_component.h` |
| 签名 | 与 Node::CreateWriter 相同 |
| 参数 | 同 Node::CreateWriter |
| 返回值 | 同 Node::CreateWriter |

### SEGAR_REGISTER_COMPONENT

| 字段 | 说明 |
|------|------|
| 简介 | 将组件类注册到框架，DAG 才能加载。必须放在类定义之外、全局作用域 |
| 所属 | 宏（`segar/component/component.h` 等） |
| 命名空间 | 无（宏展开为全局） |
| 头文件 | `segar/component/component.h` |
| 签名 | `SEGAR_REGISTER_COMPONENT(ClassName)` |
| 参数 | `ClassName`：组件类名 |
| 返回值 | — |

---

## 9. 日志

流式日志宏（AINFO / AWARN / AERROR）及条件输出。**头文件**：`segar/segar.h`，**命名空间**：`rti::segar`

| 宏 | 简介 |
|------|------|
| **AINFO** | 信息级别日志流，用法如 `AINFO << "message";` |
| **AWARN** | 警告级别日志流 |
| **AERROR** | 错误级别日志流 |
| **AINFO_IF(cond)** | 条件为真时输出信息，用法如 `AINFO_IF(cond) << "message";` |
