# Segar API Reference

## Introduction

This document is intended for C++ developers who need a quick reference for common Segar framework APIs. Each API is presented in a table with the following fields: **Summary**, **Belongs To** (namespace/class), **Namespace**, **Header**, **Signature**, **Parameters**, and **Return Value**.

---

## 1. Core Initialization

Required APIs for process entry: runtime initialization, node creation, and blocking wait. **Header**: `segar/segar.h`

### Init

| Field | Description |
|------|------|
| Summary | Required Segar runtime initialization function at process entry |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool Init(const char* argv0)` |
| Parameters | `argv0`: process entry argument, typically `argv[0]`, used to initialize the Segar runtime |
| Return Value | `true` on success, `false` on failure. This must be called at process entry; exit on failure |

### CreateNode

| Field | Description |
|------|------|
| Summary | Creates a Segar node, which is the carrier object for creating Writer/Reader/Service/Client/ActionServer/ActionClient |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<Node> CreateNode(const std::string& name)` |
| Parameters | `name`: node name, must be unique within the same process |
| Return Value | Node smart pointer; null pointer on failure |

### WaitForShutdown

| Field | Description |
|------|------|
| Summary | Blocks the current thread until a shutdown signal is received (for example, Ctrl+C), commonly used to keep the main thread alive |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void WaitForShutdown()` |
| Parameters | None |
| Return Value | None |

---

## 2. Topic Publish/Subscribe

Topic-based pub/sub communication, supporting Writer publish and Reader callback subscription.

### CreateWriter

| Field | Description |
|------|------|
| Summary | Creates a Topic publisher to send messages to a specified Topic |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<Writer<T>> CreateWriter<T>(const std::string& topic_name)` |
| Parameters | `topic_name`: Topic name; must exactly match the subscriber side, e.g. `"/topic/chatter"`; `T`: template message type (defined in `.msg`) |
| Return Value | Writer smart pointer; null pointer on failure |

### Write

| Field | Description |
|------|------|
| Summary | Publishes one message to a Topic |
| Belongs To | `Writer<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool Write(const std::shared_ptr<T>& msg)` |
| Parameters | `msg`: message to publish, of type `std::shared_ptr<T>` |
| Return Value | `true` if sent successfully, `false` otherwise |

### CreateReader

| Field | Description |
|------|------|
| Summary | Creates a Topic subscriber that receives messages via callback |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<Reader<T>> CreateReader<T>(const std::string& topic_name, Callback callback[, options])` |
| Parameters | `topic_name`: Topic name; must exactly match the publisher side; `callback`: callback invoked when messages are received; `options`: (optional) such as `pending_queue_size` (default 5), QoS, etc. |
| Return Value | Reader smart pointer; null pointer on failure |

---

## 3. Service Calls

Classic request-response model: Service handles requests and returns responses; Client supports synchronous and asynchronous calls.

### CreateService

| Field | Description |
|------|------|
| Summary | Creates a service server and registers a request handler callback to respond to client requests |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<Service<T>> CreateService<T>(const std::string& service_name, ServiceCallback callback)` |
| Parameters | `service_name`: service name, must exactly match the client side; `callback`: request handler callback that must populate `response`; `T`: service type (defined in `.srv`) |
| Return Value | Service smart pointer; null pointer on failure |

### CreateClient

| Field | Description |
|------|------|
| Summary | Creates a service client used to send requests to the service server |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<Client<T>> CreateClient<T>(const std::string& service_name)` |
| Parameters | `service_name`: service name, must exactly match the server side; `T`: service type |
| Return Value | Client smart pointer; null pointer on failure |

### SyncSendRequest

| Field | Description |
|------|------|
| Summary | Sends a service request synchronously and waits for the response |
| Belongs To | `Client<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<T::Response> SyncSendRequest(const std::shared_ptr<T::Request>& request)` |
| Parameters | `request`: request object |
| Return Value | Response smart pointer; non-null on success, null on failure or timeout |

### AsyncSendRequest

| Field | Description |
|------|------|
| Summary | Sends a service request asynchronously and receives the response via callback |
| Belongs To | `Client<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void AsyncSendRequest(const std::shared_ptr<T::Request>& request, ResponseCallback callback)` |
| Parameters | `request`: request object; `callback`: signature `void(const std::shared_ptr<T::Response>& response)`, where null `response` indicates request failure |
| Return Value | None |

---

## 4. Action Execution

Long-running actions (Goal/Feedback/Result) with cancellation and progress feedback. Client supports both synchronous and asynchronous calling modes.

### CreateActionServer

| Field | Description |
|------|------|
| Summary | Creates an Action server to handle goal acceptance, cancellation, and execution |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<ActionServer<T>> CreateActionServer<T>(const std::string& action_name, const ActionServer<T>::Callbacks& callbacks)` |
| Parameters | `action_name`: Action name; `callbacks`: includes `on_goal`, `on_cancel`, and `on_execute`; `T`: Action type (defined in `.action`) |
| Return Value | ActionServer smart pointer; null pointer on failure |

### PublishFeedback

| Field | Description |
|------|------|
| Summary | Sends Action execution progress feedback to clients |
| Belongs To | `ActionServer<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void PublishFeedback(const GoalID& goal_id, const std::shared_ptr<T::Feedback>& feedback)` |
| Parameters | `goal_id`: unique identifier of the current goal; `feedback`: feedback message |
| Return Value | None |

### CancelGoal

| Field | Description |
|------|------|
| Summary | Cancels a specified goal and finishes it with a result |
| Belongs To | `ActionServer<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void CancelGoal(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| Parameters | `goal_id`: goal identifier to cancel; `result`: result returned on cancellation (can include partial data) |
| Return Value | None |

### Succeed

| Field | Description |
|------|------|
| Summary | Marks a goal as successfully completed and returns a result |
| Belongs To | `ActionServer<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void Succeed(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| Parameters | `goal_id`: completed goal identifier; `result`: final result |
| Return Value | None |

### CreateActionClient

| Field | Description |
|------|------|
| Summary | Creates an Action client for sending goals and receiving feedback/result |
| Belongs To | `Node` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::shared_ptr<ActionClient<T>> CreateActionClient<T>(const std::string& action_name[, const GoalCallbacks& callbacks])` |
| Parameters | `action_name`: Action name; `callbacks`: (optional) GoalCallbacks including `on_accept`, `on_result`, `on_cancel`, and `on_feedback` |
| Return Value | ActionClient smart pointer; null pointer on failure |

### SyncSendGoal

| Field | Description |
|------|------|
| Summary | Sends a goal synchronously and blocks until the server accepts it or fails |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool SyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| Parameters | `goal`: goal object to send; `goal_id`: output parameter receiving the unique goal identifier |
| Return Value | `true` if sent successfully and accepted by the server, `false` on failure |

### SyncCancelGoal

| Field | Description |
|------|------|
| Summary | Sends a cancellation request synchronously and waits for handling result |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool SyncCancelGoal(const GoalID& goal_id)` |
| Parameters | `goal_id`: goal identifier to cancel |
| Return Value | `true` if the cancellation request is sent and handled, `false` on failure |

### WaitForResult (Overload 1)

| Field | Description |
|------|------|
| Summary | Waits for the result of a specified goal |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool WaitForResult(const GoalID& goal_id)` |
| Parameters | `goal_id`: goal identifier to wait for |
| Return Value | `true` if a result is received before timeout, `false` on timeout |

### WaitForResult (Overload 2)

| Field | Description |
|------|------|
| Summary | Waits for the result of a specified goal and retrieves both Result and status code |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool WaitForResult(const GoalID& goal_id, T::Result* result, GoalStatusCode* status)` |
| Parameters | `goal_id`: goal identifier to wait for; `result`: output parameter for Result; `status`: output parameter for status code (e.g. STATUS_SUCCEEDED, STATUS_CANCELED, STATUS_ABORTED) |
| Return Value | `true` if a result is received before timeout, `false` on timeout |

### AsyncSendGoal

| Field | Description |
|------|------|
| Summary | Sends a goal asynchronously without waiting for server acceptance |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `bool AsyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| Parameters | `goal`: goal object to send; `goal_id`: output parameter receiving the goal identifier |
| Return Value | `true` if sent successfully, `false` if send failed (for example, server unavailable) |

### AsyncCancelGoal

| Field | Description |
|------|------|
| Summary | Sends a cancellation request asynchronously without waiting for completion |
| Belongs To | `ActionClient<T>` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void AsyncCancelGoal(const GoalID& goal_id)` |
| Parameters | `goal_id`: goal identifier to cancel |
| Return Value | None |

---

## 5. Parameters

Node-level parameter management: local load/set/get/dump, plus remote parameter get/load for other nodes.

### Segar_Load_Local_Params

| Field | Description |
|------|------|
| Summary | Loads local parameters into a node from a YAML or dump file |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` (used together with `segar/segar.h`) |
| Signature | `bool Segar_Load_Local_Params(Node* node, const std::string& path)` |
| Parameters | `node`: node pointer; `path`: YAML or dump file path |
| Return Value | `true` on success, `false` on failure |

### Segar_List_Local_Params

| Field | Description |
|------|------|
| Summary | Lists all local parameters on a node |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_List_Local_Params(Node* node, std::vector<Parameter>* out_list)` |
| Parameters | `node`: node pointer; `out_list`: output parameter receiving the parameter list |
| Return Value | `true` on success, `false` on failure |

### Segar_Set_Local_Param

| Field | Description |
|------|------|
| Summary | Sets a local parameter on a node (Value supports int, string, Protobuf, etc.) |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Set_Local_Param(Node* node, const std::string& name, const Value& value)` |
| Parameters | `node`: node pointer; `name`: parameter name; `value`: parameter value |
| Return Value | `true` on success, `false` on failure |

### Segar_Get_Local_Param

| Field | Description |
|------|------|
| Summary | Gets the value of a local parameter on a node |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Get_Local_Param(Node* node, const std::string& name, Value* out_value)` |
| Parameters | `node`: node pointer; `name`: parameter name; `out_value`: output parameter receiving the parameter value |
| Return Value | `true` on success, `false` on failure |

### Segar_Dump_Local_Params

| Field | Description |
|------|------|
| Summary | Dumps local node parameters to a file |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Dump_Local_Params(Node* node, const std::string& path)` |
| Parameters | `node`: node pointer; `path`: output file path |
| Return Value | `true` on success, `false` on failure |

### Segar_Get_Remote_Param

| Field | Description |
|------|------|
| Summary | Gets a parameter value from a remote node |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Get_Remote_Param(const std::string& node_name, const std::string& name, Value* out_value)` |
| Parameters | `node_name`: remote node name; `name`: parameter name; `out_value`: output parameter |
| Return Value | `true` on success, `false` on failure |

### Segar_Load_Remote_Params

| Field | Description |
|------|------|
| Summary | Loads parameters from a file into a remote node |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Load_Remote_Params(const std::string& node_name, const std::string& path)` |
| Parameters | `node_name`: remote node name; `path`: YAML or dump file path |
| Return Value | `true` on success, `false` on failure |

### Segar_List_Remote_Params

| Field | Description |
|------|------|
| Summary | Lists all parameters on a remote node |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_List_Remote_Params(const std::string& node_name, std::vector<Parameter>* out_list)` |
| Parameters | `node_name`: remote node name; `out_list`: output parameter |
| Return Value | `true` on success, `false` on failure |

### Segar_Dump_Remote_Params

| Field | Description |
|------|------|
| Summary | Dumps remote node parameters to a file |
| Belongs To | C-style API |
| Namespace | None (global C-style function) |
| Header | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Dump_Remote_Params(const std::string& node_name, const std::string& path)` |
| Parameters | `node_name`: remote node name; `path`: output file path |
| Return Value | `true` on success, `false` on failure |

---

## 6. Timer

Periodic timer based on millisecond intervals, with configurable callback and auto-start behavior.

### Timer Constructor

| Field | Description |
|------|------|
| Summary | Creates a periodic timer that invokes the callback at the specified interval |
| Belongs To | `rti::segar::Timer` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `Timer(int interval_ms, std::function<void()> callback, bool auto_start)` |
| Parameters | `interval_ms`: execution interval in milliseconds; `callback`: no-argument callback executed on trigger; `auto_start`: whether to start automatically after construction |
| Return Value | None (constructor) |

### Start

| Field | Description |
|------|------|
| Summary | Starts the timer and begins periodic callback execution |
| Belongs To | `rti::segar::Timer` class |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void Start()` |
| Parameters | None |
| Return Value | None |

---

## 7. Concurrency Primitives

Asynchronous execution, coroutine synchronization (TaskEvent), locking, yielding, and sleeping for multi-task and thread-safe scenarios.

### Async

| Field | Description |
|------|------|
| Summary | Executes a callable asynchronously and returns a future for waiting on result |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `std::future<R> Async(Callable&& callable)` (where `R` is the return type of `callable`) |
| Parameters | `callable`: callable object (function, lambda, etc.) |
| Return Value | `std::future`, which can be used with `wait()`, `get()`, etc. |

### Execute

| Field | Description |
|------|------|
| Summary | Executes a callable asynchronously in fire-and-forget mode, without waiting for completion |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/segar.h` |
| Signature | `void Execute(Callable&& callable, Args&&... args)` |
| Parameters | `callable`: callable object; `args`: arguments passed to `callable` |
| Return Value | None |

### TaskEvent

| Field | Description |
|------|------|
| Summary | Event synchronization object used for coroutine-to-coroutine notification |
| Belongs To | `rti::segar::TaskEvent` class |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | class type |
| Parameters | — |
| Return Value | — |

### Notify

| Field | Description |
|------|------|
| Summary | Notifies all coroutines waiting on this TaskEvent |
| Belongs To | `TaskEvent` class |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | `void Notify()` |
| Parameters | None |
| Return Value | None |

### Wait

| Field | Description |
|------|------|
| Summary | Waits for TaskEvent notification with timeout support |
| Belongs To | `TaskEvent` class |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | `bool Wait(std::chrono::duration timeout)` |
| Parameters | `timeout`: timeout duration |
| Return Value | `true` if notified before timeout, `false` on timeout |

### LockGuard

| Field | Description |
|------|------|
| Summary | RAII-style mutex guard: locks on construction and unlocks on destruction; coroutine-safe |
| Belongs To | `rti::segar::LockGuard<Mutex>` template class |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | `LockGuard(Mutex& mutex)` |
| Parameters | `mutex`: mutex reference (for example, `std::mutex`) |
| Return Value | None |

### Yield

| Field | Description |
|------|------|
| Summary | Yields execution of the current coroutine to avoid starving other coroutines |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | `void Yield()` |
| Parameters | None |
| Return Value | None |

### SleepFor

| Field | Description |
|------|------|
| Summary | Coroutine-safe sleep; yields execution while sleeping |
| Belongs To | `rti::segar` namespace |
| Namespace | `rti::segar` |
| Header | `segar/task/task.h` |
| Signature | `void SleepFor(std::chrono::duration d)` |
| Parameters | `d`: sleep duration |
| Return Value | None |

---

## 8. Components

DAG-based component framework: event-triggered components (`Component`) and timer-triggered components (`TimerComponent`), registered via `SEGAR_REGISTER_COMPONENT`.

### Component Base Class

| Field | Description |
|------|------|
| Summary | Base class for event-triggered components; template parameters are input message types and must match the reader order in DAG |
| Belongs To | `rti::segar::Component<InputType1, InputType2, ...>` template class |
| Namespace | `rti::segar` |
| Header | `segar/component/component.h` |
| Signature | template class |
| Parameters | — |
| Return Value | — |

### TimerComponent Base Class

| Field | Description |
|------|------|
| Summary | Base class for timer-triggered components; `Proc` is invoked periodically using DAG `interval` |
| Belongs To | `rti::segar::TimerComponent` class |
| Namespace | `rti::segar` |
| Header | `segar/component/timer_component.h` |
| Signature | class type |
| Parameters | — |
| Return Value | — |

### Init (Virtual Function)

| Field | Description |
|------|------|
| Summary | Component initialization entry; component will not start if it returns false |
| Belongs To | `Component` / `TimerComponent` base class |
| Namespace | `rti::segar` |
| Header | `segar/component/component.h` or `segar/component/timer_component.h` |
| Signature | `virtual bool Init()` |
| Parameters | None |
| Return Value | `true` for successful initialization, `false` for failure |

### Proc (Virtual Function)

| Field | Description |
|------|------|
| Summary | Business logic entry: `Component` is event-triggered, `TimerComponent` is interval-triggered |
| Belongs To | `Component` / `TimerComponent` base class |
| Namespace | `rti::segar` |
| Header | `segar/component/component.h` or `segar/component/timer_component.h` |
| Signature | `Component`: `virtual bool Proc(const std::shared_ptr<Input1>& msg1, ...)`; `TimerComponent`: `virtual bool Proc()` |
| Parameters | `Component::Proc` receives messages mapped one-to-one with DAG readers; `TimerComponent::Proc` takes no parameters |
| Return Value | `true` for success, `false` for failure (the framework records errors) |

### CreateWriter (Inside Components)

| Field | Description |
|------|------|
| Summary | Inside components, create a publisher using `node_->CreateWriter<T>(topic)` |
| Belongs To | `node_` member provided by `Component` / `TimerComponent` base class |
| Namespace | `rti::segar` |
| Header | `segar/component/component.h` or `segar/component/timer_component.h` |
| Signature | Same as `Node::CreateWriter` |
| Parameters | Same as `Node::CreateWriter` |
| Return Value | Same as `Node::CreateWriter` |

### SEGAR_REGISTER_COMPONENT

| Field | Description |
|------|------|
| Summary | Registers a component class with the framework so it can be loaded by DAG. Must be placed outside class definitions in global scope |
| Belongs To | Macro (declared in `segar/component/component.h`, etc.) |
| Namespace | None (macro expands to global scope) |
| Header | `segar/component/component.h` |
| Signature | `SEGAR_REGISTER_COMPONENT(ClassName)` |
| Parameters | `ClassName`: component class name |
| Return Value | — |

---

## 9. Logging

Stream-style logging macros (`AINFO`, `AWARN`, `AERROR`) plus conditional logging. **Header**: `segar/segar.h`, **Namespace**: `rti::segar`

| Macro | Summary |
|------|------|
| **AINFO** | Info-level log stream, e.g. `AINFO << "message";` |
| **AWARN** | Warning-level log stream |
| **AERROR** | Error-level log stream |
| **AINFO_IF(cond)** | Emits info only when condition is true, e.g. `AINFO_IF(cond) << "message";` |
