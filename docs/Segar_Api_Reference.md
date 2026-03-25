# Segar API Reference Manual

## Introduction

This document is suitable for C++ developers to quickly review the common interfaces of the Segar framework. Each API includes: **Introduction**, **Official** (namespace/class), **Namespace**, **Header file**, **Signature**, **Parameters**, **Return value**, displayed in table form.

---

## 1. Basics and initialization

Necessary initialization, node creation and blocking wait for program entry. **Header file**: `segar/segar.h`

### Init

| Field | Description |
|------|------|
| Introduction | Segar runtime initialization function that must be called at program entry |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool Init(const char* argv0)` |
| Parameters | `argv0`: program entry parameter, usually passed `argv[0]`, used to initialize Segar runtime |
| Return value | `true` means initialization is successful, `false` means failure. The program entry must be called, and it should exit when it fails |

### CreateNode

| Field | Description |
|------|------|
| Introduction | Create Segar node, which is the carrier for creating Writer/Reader/Service/Client/ActionServer/ActionClient |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<Node> CreateNode(const std::string& name)` |
| Parameters | `name`: node name, must be unique in the same process |
| Return value | Node smart pointer, returns null pointer on failure |

### WaitForShutdown

| Field | Description |
|------|------|
| Introduction | Block the current thread until it receives an exit signal (such as Ctrl+C), often used to keep the main thread alive |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void WaitForShutdown()` |
| Parameters | None |
| Return value | None |

---

## 2. Topic publish and subscribe

Topic-based publish/subscribe communication supports Writer publishing and Reader subscription callbacks.

### CreateWriter

| Field | Description |
|------|------|
| Introduction | Create a Topic publisher to send messages to the specified Topic |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<Writer<T>> CreateWriter<T>(const std::string& topic_name)` |
| Parameters | `topic_name`: Topic name, which must be exactly the same as the subscriber, such as `"/topic/chatter"`; `T`: template parameter, message type (from .msg definition) |
| Return value | Writer smart pointer, returns null pointer on failure |

### Write

| Field | Description |
|------|------|
| Introduction | Post a message to Topic |
| Belongs to | `Writer<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool Write(const std::shared_ptr<T>& msg)` |
| Parameters | `msg`: The message to be published, type is `std::shared_ptr<T>` |
| Return value | `true` means sending successfully, `false` means failure |

### CreateReader

| Field | Description |
|------|------|
| Introduction | Create a Topic subscriber and receive messages through callbacks |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<Reader<T>> CreateReader<T>(const std::string& topic_name, Callback callback[, options])` |
| Parameters | `topic_name`: Topic name, which must be exactly the same as the publisher; `callback`: callback when receiving a message; `options`: (optional) such as `pending_queue_size` (default 5), qos, etc. |
| Return value | Reader smart pointer, returns null pointer on failure |

---

## 3. Service service call

In the typical request-response model, the Service side processes the request and returns a Response, and the Client side makes a synchronous or asynchronous call.

### CreateService

| Field | Description |
|------|------|
| Introduction | Create a server and register request processing callbacks to respond to client requests |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<Service<T>> CreateService<T>(const std::string& service_name, ServiceCallback callback)` |
| Parameters | `service_name`: service name, must be exactly the same as the client; `callback`: callback for processing requests, response needs to be filled in the callback; `T`: service type (from .srv definition) |
| Return value | Service smart pointer, returns null pointer on failure |

### CreateClient

| Field | Description |
|------|------|
| Introduction | Create a client to initiate requests to the server |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<Client<T>> CreateClient<T>(const std::string& service_name)` |
| Parameters | `service_name`: service name, which must be exactly the same as the server; `T`: service type |
| Return value | Client smart pointer, returns null pointer on failure |

### SyncSendRequest| Field | Description |
|------|------|
| Introduction | Send service requests synchronously and wait for responses |
| Belongs to | `Client<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<T::Response> SyncSendRequest(const std::shared_ptr<T::Request>& request)` |
| Parameters | `request`: request object |
| Return value | Response smart pointer, non-null on success, returns null pointer on failure or timeout |

### AsyncSendRequest

| Field | Description |
|------|------|
| Introduction | Send service requests asynchronously and receive responses through callbacks |
| Belongs to | `Client<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void AsyncSendRequest(const std::shared_ptr<T::Request>& request, ResponseCallback callback)` |
| Parameters | `request`: request object; `callback`: signature is `void(const std::shared_ptr<T::Response>& response)`, an empty response means the request failed |
| Return value | None |

---

## 4. Action action execution

Long-running actions (Goal/Feedback/Result) support cancellation and progress feedback. Client supports synchronous and asynchronous calls.

### CreateActionServer

| Field | Description |
|------|------|
| Introduction | Create an Action server to handle goal reception, cancellation and execution |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<ActionServer<T>> CreateActionServer<T>(const std::string& action_name, const ActionServer<T>::Callbacks& callbacks)` |
| Parameters | `action_name`: Action name; `callbacks`: Contains three callbacks: `on_goal`, `on_cancel`, `on_execute`; `T`: Action type (from .action definition) |
| Return value | ActionServer smart pointer, returns null pointer on failure |

### PublishFeedback

| Field | Description |
|------|------|
| Introduction | Send Action execution progress feedback to the client |
| Belongs to | `ActionServer<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void PublishFeedback(const GoalID& goal_id, const std::shared_ptr<T::Feedback>& feedback)` |
| Parameters | `goal_id`: the unique identifier of the current goal; `feedback`: feedback message |
| Return value | None |

### CancelGoal

| Field | Description |
|------|------|
| Introduction | Unspecify goal and end with result |
| Belongs to | `ActionServer<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void CancelGoal(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| Parameters | `goal_id`: the goal identifier to be canceled; `result`: the result of cancellation (may contain partial data) |
| Return value | None |

### Succeed

| Field | Description |
|------|------|
| Introduction | Mark goal completed successfully and return result |
| Belongs to | `ActionServer<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void Succeed(const GoalID& goal_id, const std::shared_ptr<T::Result>& result)` |
| Parameters | `goal_id`: completed goal identification; `result`: final result |
| Return value | None |

### CreateActionClient

| Field | Description |
|------|------|
| Introduction | Create an Action client to send goal and receive feedback/result |
| Belongs to | `Node` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::shared_ptr<ActionClient<T>> CreateActionClient<T>(const std::string& action_name[, const GoalCallbacks& callbacks])` |
| Parameters | `action_name`: Action name; `callbacks`: (optional) GoalCallbacks, including `on_accept`, `on_result`, `on_cancel`, `on_feedback` |
| Return value | ActionClient smart pointer, returns null pointer on failure |

### SyncSendGoal

| Field | Description |
|------|------|
| Introduction | Send goal synchronously, blocking until the server accepts or fails |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool SyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| Parameters | `goal`: the goal object to be sent; `goal_id`: output parameter, used to receive the unique identifier of the goal |
| Return value | `true` means successfully sent and accepted by the server, `false` means failure |

### SyncCancelGoal| Field | Description |
|------|------|
| Introduction | Send a cancellation request synchronously and wait for the processing result |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool SyncCancelGoal(const GoalID& goal_id)` |
| Parameters | `goal_id`: the goal identifier to be canceled |
| Return value | `true` means the cancellation request has been sent and processed, `false` means it failed |

### WaitForResult (overload 1)

| Field | Description |
|------|------|
| Introduction | Waiting for the result of the specified goal |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool WaitForResult(const GoalID& goal_id)` |
| Parameters | `goal_id`: the goal identifier to wait for |
| Return value | `true` means result is received before timeout, `false` means timeout |

### WaitForResult (overload 2)

| Field | Description |
|------|------|
| Introduction | Wait for the result of the specified goal and obtain the Result and status code |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool WaitForResult(const GoalID& goal_id, T::Result* result, GoalStatusCode* status)` |
| Parameters | `goal_id`: waiting goal identifier; `result`: output parameter, receive Result; `status`: output parameter, receive status code (such as STATUS_SUCCEEDED, STATUS_CANCELED, STATUS_ABORTED) |
| Return value | `true` means result is received before timeout, `false` means timeout |

### AsyncSendGoal

| Field | Description |
|------|------|
| Introduction | Send goal asynchronously without waiting for the server to accept it |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `bool AsyncSendGoal(const T::Goal& goal, GoalID* goal_id)` |
| Parameters | `goal`: the goal object to be sent; `goal_id`: output parameter, receiving the goal identifier |
| Return value | `true` means successful sending, `false` means sending failed (such as unable to connect to the server) |

### AsyncCancelGoal

| Field | Description |
|------|------|
| Introduction | Send cancellation request asynchronously without waiting for processing to complete |
| Belongs to | `ActionClient<T>` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void AsyncCancelGoal(const GoalID& goal_id)` |
| Parameters | `goal_id`: the goal identifier to be canceled |
| Return value | None |

---

## 5. Parameter parameter

Node-level parameter management: local loading/setting/getting/exporting, as well as getting and loading remote node parameters.

### Segar_Load_Local_Params

| Field | Description |
|------|------|
| Introduction | Loading local parameters into nodes from YAML or dump files |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` (needs to be used with `segar/segar.h`) |
| Signature | `bool Segar_Load_Local_Params(Node* node, const std::string& path)` |
| Parameters | `node`: node pointer; `path`: YAML or dump file path |
| Return value | `true` means success, `false` means failure |

### Segar_List_Local_Params

| Field | Description |
|------|------|
| Introduction | List all local parameters of a node |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_List_Local_Params(Node* node, std::vector<Parameter>* out_list)` |
| Parameters | `node`: node pointer; `out_list`: output parameters, receive parameter list |
| Return value | `true` means success, `false` means failure |

### Segar_Set_Local_Param

| Field | Description |
|------|------|
| Introduction | Set the local parameters of the node (Value supports int, string, Protobuf, etc.) |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Set_Local_Param(Node* node, const std::string& name, const Value& value)` |
| Parameters | `node`: node pointer; `name`: parameter name; `value`: parameter value |
| Return value | `true` means success, `false` means failure |

### Segar_Get_Local_Param

| Field | Description |
|------|------|
| Introduction | Get the local parameter value of the node |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Get_Local_Param(Node* node, const std::string& name, Value* out_value)` |
| Parameters | `node`: node pointer; `name`: parameter name; `out_value`: output parameter, receive parameter value |
| Return value | `true` means success, `false` means failure |

### Segar_Dump_Local_Params| Field | Description |
|------|------|
| Introduction | Export a node's local parameters to a file |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Dump_Local_Params(Node* node, const std::string& path)` |
| Parameters | `node`: node pointer; `path`: save file path |
| Return value | `true` means success, `false` means failure |

### Segar_Get_Remote_Param

| Field | Description |
|------|------|
| Introduction | Get the parameter value of the remote node |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Get_Remote_Param(const std::string& node_name, const std::string& name, Value* out_value)` |
| Parameters | `node_name`: remote node name; `name`: parameter name; `out_value`: output parameter |
| Return value | `true` means success, `false` means failure |

### Segar_Load_Remote_Params

| Field | Description |
|------|------|
| Introduction | Load parameters from file to remote node |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Load_Remote_Params(const std::string& node_name, const std::string& path)` |
| Parameters | `node_name`: remote node name; `path`: YAML or dump file path |
| Return value | `true` means success, `false` means failure |

### Segar_List_Remote_Params

| Field | Description |
|------|------|
| Introduction | List all parameters of the remote node |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_List_Remote_Params(const std::string& node_name, std::vector<Parameter>* out_list)` |
| Parameters | `node_name`: remote node name; `out_list`: output parameters |
| Return value | `true` means success, `false` means failure |

### Segar_Dump_Remote_Params

| Field | Description |
|------|------|
| Introduction | Export the parameters of a remote node to a file |
| Belongs to | C interface |
| namespace | None (global C-style functions) |
| Header file | `segar/parameter/segar_parameter_api.h` |
| Signature | `bool Segar_Dump_Remote_Params(const std::string& node_name, const std::string& path)` |
| Parameters | `node_name`: remote node name; `path`: save file path |
| Return value | `true` means success, `false` means failure |

---

## 6. Timer timer

A periodic timer based on millisecond intervals, you can specify a callback and whether to start automatically.

### Timer constructor

| Field | Description |
|------|------|
| Introduction | Create a periodic timer and call back at specified intervals |
| Belongs to | `rti::segar::Timer` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `Timer(int interval_ms, std::function<void()> callback, bool auto_start)` |
| Parameters | `interval_ms`: execution interval, in milliseconds; `callback`: no parameter callback, executed when it expires; `auto_start`: whether to start automatically after construction |
| Return value | None (constructor) |

### Start

| Field | Description |
|------|------|
| Introduction | Start the timer and start calling callback periodically at intervals |
| Belongs to | `rti::segar::Timer` class |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void Start()` |
| Parameters | None |
| Return value | None |

---

## 7. Concurrency infrastructure

Asynchronous execution, coroutine synchronization (TaskEvent), lock, yield and sleep are used in multi-tasking and thread-safety scenarios.

### Async

| Field | Description |
|------|------|
| Introduction | Execute a callable object asynchronously and return a future to wait for the result |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `std::future<R> Async(Callable&& callable)` (R is callable return type) |
| Parameters | `callable`: callable object (function, lambda, etc.) |
| Return value | `std::future`, can be used for `wait()`, `get()` to wait for the result |

### Execute

| Field | Description |
|------|------|
| Introduction | Asynchronous execution of callable objects, fire-and-forget, without waiting for execution to complete |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/segar.h` |
| Signature | `void Execute(Callable&& callable, Args&&... args)` |
| Parameters | `callable`: callable object; `args`: parameters passed to callable |
| Return value | None |

### TaskEvent

| Field | Description |
|------|------|
| Introduction | Event synchronization object, used for notification between coroutines |
| Belongs to | `rti::segar::TaskEvent` class |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| signature | class type |
| Parameters | — |
| Return value | — |

### Notify| Field | Description |
|------|------|
| Introduction | Notify all coroutines waiting for this TaskEvent |
| Belongs to | `TaskEvent` class |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| Signature | `void Notify()` |
| Parameters | None |
| Return value | None |

### Wait

| Field | Description |
|------|------|
| Introduction | Wait for TaskEvent notification, support timeout |
| Belongs to | `TaskEvent` class |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| Signature | `bool Wait(std::chrono::duration timeout)` |
| Parameters | `timeout`: timeout period |
| Return value | `true` means receiving notification before timeout, `false` means timeout |

### LockGuard

| Field | Description |
|------|------|
| Introduction | RAII style mutex lock, locked during construction and unlocked during destruction, coroutine security |
| Belongs to | `rti::segar::LockGuard<Mutex>` template class |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| Signature | `LockGuard(Mutex& mutex)` |
| Parameters | `mutex`: mutex reference (such as `std::mutex`) |
| Return value | None |

### Yield

| Field | Description |
|------|------|
| Introduction | Give up the execution rights of the current coroutine to prevent long-term occupation from starving other coroutines |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| Signature | `void Yield()` |
| Parameters | None |
| Return value | None |

### SleepFor

| Field | Description |
|------|------|
| Introduction | Coroutine safe sleep, giving up execution rights during sleep |
| Belongs to | `rti::segar` namespace |
| namespace | `rti::segar` |
| Header file | `segar/task/task.h` |
| Signature | `void SleepFor(std::chrono::duration d)` |
| Parameters | `d`: sleep duration |
| Return value | None |

---

## 8. Component

DAG-based component framework: event triggering (Component), timer triggering (TimerComponent) and multi-channel message synchronization triggering (SyncComponent), which need to be registered with `SEGAR_REGISTER_COMPONENT`.

### Component base class

| Field | Description |
|------|------|
| Introduction | The base class of event triggering components. The template parameter is the input message type, which is consistent with the order of readers in DAG |
| Belongs to | `rti::segar::Component<InputType1, InputType2, ...>` Template class |
| namespace | `rti::segar` |
| Header file | `segar/component/component.h` |
| Signature | Template class |
| Parameters | — |
| Return value | — |

### TimerComponent base class

| Field | Description |
|------|------|
| Introduction | The timer trigger component base class calls Proc periodically according to the interval in the DAG |
| Belongs to | `rti::segar::TimerComponent` class |
| namespace | `rti::segar` |
| Header file | `segar/component/timer_component.h` |
| signature | class type |
| Parameters | — |
| Return value | — |

### SyncComponent base class

| Field | Description |
|------|------|
| Introduction | Multi-topic synchronization component base class, triggers Proc after aligning/fusion of multiple inputs according to timestamp |
| Belongs to | `rti::segar::SyncComponent<M0, M1, ...>` template class |
| namespace | `rti::segar` |
| Header file | `segar/component/sync_component.h` |
| Signature | Template class |
| Parameters | — |
| Return value | — |

### Init (virtual function)

| Field | Description |
|------|------|
| Introduction | Component initialization entry, the component will not start when false is returned |
| Belongs to | `Component` / `TimerComponent` / `SyncComponent` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/component.h` or `segar/component/timer_component.h` or `segar/component/sync_component.h` |
| Signature | `virtual bool Init()` |
| Parameters | None |
| Return value | `true` means initialization is successful, `false` means failure |

### Proc (virtual function)

| Field | Description |
|------|------|
| Introduction | Component business logic entry, Component is triggered by events, TimerComponent is triggered by interval, and SyncComponent is triggered by synchronization results |
| Belongs to | `Component` / `TimerComponent` / `SyncComponent` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/component.h` or `segar/component/timer_component.h` or `segar/component/sync_component.h` |
| Signature | `Component`: `virtual bool Proc(const std::shared_ptr<Input1>& msg1, ...)`; `TimerComponent`: `virtual bool Proc()`; `SyncComponent`: `virtual bool Proc(const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| Parameters | Proc of Component/SyncComponent receives messages corresponding to readers; TimerComponent has no parameters |
| Return value | `true` indicates successful processing, `false` indicates failure (the framework will log the error) |

### GetTimeStamp(virtual function, SyncComponent)| Field | Description |
|------|------|
| Introduction | Provides message timestamp to synchronizer for alignment and timeout calculation; needs to be implemented by SyncComponent subclass |
| Belongs to | `SyncComponent` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/sync_component.h` |
| Signature | Single input: `virtual uint64_t GetTimeStamp(const std::shared_ptr<M0>& msg0)`; Multiple input: `virtual uint64_t GetTimeStamp(size_t index, const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| Parameters | `index`: message index (when multiple inputs are made); `msg*`: each input message |
| Return value | Timestamp (usually the message time, the unit is defined by the business, commonly nanoseconds) |

### TimeoutProc (optional virtual function, SyncComponent)

| Field | Description |
|------|------|
| Introduction | Triggered when the processing conditions are not met or a timeout occurs during the synchronization cycle. The default implementation is empty and can be used for downgrade/alarm logic |
| Belongs to | `SyncComponent` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/sync_component.h` |
| Signature | `virtual void TimeoutProc()` |
| Parameters | None |
| Return value | None |

### Fusionable (optional virtual function, SyncComponent multiple inputs)

| Field | Description |
|------|------|
| Introduction | Fusion feasibility judgment hook before processing, when returning `false`, the current group message will not enter Proc |
| Belongs to | `SyncComponent<M0, M1, ...>` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/sync_component.h` |
| Signature | `virtual bool Fusionable(const std::shared_ptr<M0>& msg0, const std::shared_ptr<M1>& msg1, ...)` |
| Parameters | Multiple input messages aligned with Proc |
| Return value | `true` means it can be processed; `false` means skipping the fusion result |

### CreateWriter (within component)

| Field | Description |
|------|------|
| Introduction | Create a publisher through `node_->CreateWriter<T>(topic)` in the component |
| Belongs to | `node_` member provided by `Component` / `TimerComponent` / `SyncComponent` base class |
| namespace | `rti::segar` |
| Header file | `segar/component/component.h` or `segar/component/timer_component.h` or `segar/component/sync_component.h` |
| Signature | Same as Node::CreateWriter |
| Parameters | Same as Node::CreateWriter |
| Return value | Same as Node::CreateWriter |

### SEGAR_REGISTER_COMPONENT

| Field | Description |
|------|------|
| Introduction | Register the component class to the framework so that the DAG can be loaded. Must be placed outside the class definition and in the global scope |
| Belongs to | Macros (`segar/component/component.h`, etc.) |
| Namespace | None (macro expands to global) |
| Header file | `segar/component/component.h` |
| Signature | `SEGAR_REGISTER_COMPONENT(ClassName)` |
| Parameters | `ClassName`: component class name |
| Return value | — |

---

## 9. Log

Streaming log macros (AINFO/AWARN/AERROR) and conditional output. **Header file**: `segar/segar.h`, **Namespace**: `rti::segar`

| Macro | Introduction |
|------|------|
| **AINFO** | Information level log stream, usage such as `AINFO << "message";` |
| **AWARN** | Warning level log stream |
| **AERROR** | Error level log stream |
| **AINFO_IF(cond)** | Output information when the condition is true, usage is like `AINFO_IF(cond) << "message";` |
