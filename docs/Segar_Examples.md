# Segar example description

The document includes compilation instructions, running instructions and introduction to each example.

---

## Compilation environment description

Please use this repository on ubuntu system (ubuntu 22.0.4 recommended).
The build environment requires CMake 3.10+, C++17. Please make sure pip3 is installed (can be installed using sudo apt install python3-pip).
x86 and Orin use different compilers and scripts.

### x86 compilation

- **Compiler**: GCC 9.5.0 (x86_64-linux-gnu)
- **Step**: Execute in the workspace root directory:

```bash
./scripts/build_x86.sh
```
- **Optional parameters**: `-d` compile Debug; `-r` clean the build directory; `-ra` clean build and `install/x86_64` at the same time
- **Output Directory**: `build_x86/output/`
- **Packaging**: `./scripts/pkg_x86.sh`, package `build_x86/output/` into tgz, and store it in the `build_x86` directory

### Orin cross compilation

- **Compiler**: GCC 11.4.0 (`aarch64-linux-gnu`, for Orin/ARM64)
- **Toolchain**: Use the Ubuntu 22.04 cross-compilation toolchain from the system package manager
- **Install**: `sudo apt install gcc-11-aarch64-linux-gnu g++-11-aarch64-linux-gnu`
- **Step**: Execute in the workspace root directory:

```bash
./scripts/build_orin.sh
```
- **Optional parameters**: `-d` compile Debug; `-r` clean the build directory; `-ra` clean build and `install/orin` at the same time
- **Output Directory**: `build_orin/output/`
- **Packaging**: `./scripts/pkg_orin.sh`, package `build_orin/output/` into tgz, and store it in the `build_orin` directory

---

## Run instructions

### Example run

The executable file is located in the `output` directory of each platform, for example: `build_x86/output/<example_name>/<target_name>/` or `build_orin/output/...`.

- **Single Example**: Enter the corresponding example directory and execute its `scripts/launch.sh`:

```bash
cd build_x86/output/<example_name>/<target_name>
./scripts/launch.sh
```
- **Start all examples with one click** (backend): Execute in the output directory of the compiled product, and the log is written under `logs/`:

```bash
./scripts/start_all.sh # Start all processes
./scripts/stop_all.sh # Stop all processes
./scripts/check_all.sh # View all process status
```
### segar CLI run

The `segar` command line tool is used to view/manipulate nodes, topics, services, actions, parameters, bags, etc. You need to start the example or `start_all.sh` first, and then execute it in the same output directory:

```bash
./scripts/run_segar_cli.sh
```
The script will execute sub-commands such as `segar param`, `segar node`, `segar topic`, `segar service`, `segar action`, `segar bag` and so on in sequence, and the logs are written under `logs/`.

**Run a single segar command**: Execute `source segar_setup.bash` in the output directory. After importing the environment variables, you can manually execute commands such as `segar param` respectively.

**Run tracing**: Execute `source segar_setup.bash` in the output directory. After importing the environment variables, execute `mainboard -d config/tracing_node.dag` to start tracing data collection. For first time use, `sudo tracing -i` is required to initialize MySQL; for import and query, see [Tracing Usage Guide](Segar_Tracing.md).

---

## Example list

### 1. topic_example - Topic publish and subscribe example

Demonstrates the publish and subscribe function of Topic in the Segar framework:

- **topic_talker**: Publisher example, using `Timer` to periodically publish messages to the `/topic/chatter` topic
- **topic_listener**: Subscriber example, subscribe to the `/topic/chatter` topic and receive messages

**Key Features**:

- Use `CreateWriter` to create a publisher
- Use `CreateReader` to create subscribers
- Use `Timer` to implement periodic publishing

---

### 2. service_example - Service service call example

Demonstrates the request response function of Service in the Segar framework:

- **service_server**: Server example, providing `set_camera_info` service
- **service_client_sync**: synchronous client example, using synchronous method to call services
- **service_client_async**: Asynchronous client example, using asynchronous method to call services

**Key Features**:

- Use `CreateService` to create the server
- Use `CreateClient` to create a client
- Synchronous call: `SyncSendRequest`
- Asynchronous call: `SendRequest` cooperates with callback function
- Use `Timer` to implement periodic requests

---

### 3. param_example - Parameter parameter management example

Demonstrates the parameter management function of Parameter in the Segar framework:

- **param_server**: parameter server example, using the local parameter interface (Local Parameter API)
- **param_client**: Parameter client example, using Remote Parameter API

**Key Features**:

- Local parameter operations: `Segar_Set_Local_Param`, `Segar_Get_Local_Param`, `Segar_List_Local_Params`, `Segar_Dump_Local_Params`, `Segar_Load_Local_Params`
- Remote parameter operations: `Segar_Get_Remote_Param`, `Segar_Set_Remote_Param`, `Segar_List_Remote_Params`, `Segar_Dump_Remote_Params`, `Segar_Load_Remote_Params`
- Support basic types (int, string, etc.) and Protobuf message types
- Use custom proto files to define parameter structures

---

### 4. action_example - Action action execution example

Demonstrates the long-term task execution function of Action in the Segar framework:

- **action_server**: Action server example, executes `lookup_transform` action
- **action_client_sync**: synchronous client example, using synchronous method to send the target and wait for the result
- **action_client_async**: Asynchronous client example, uses asynchronous method to send targets, and receives feedback and results through callbacks

**Key Features**:

- Use `CreateActionServer` to create an action server
- Use `CreateActionClient` to create an action client
- Support the complete life cycle of Goal, Feedback and Result
-Supports target cancellation (Cancel) function
- Synchronous call: `SyncSendGoal` + `WaitForResult`
- Asynchronous call: `AsyncSendGoal` + callback function
- Use `Timer` to implement periodic sending targets

---

### 5. component_example - Component component example

Demonstrates the component development method of Component in the Segar framework:

- **timer_component**: Example of timer component, publishing `Image` to `/topic/image_front` (front view) and `/topic/image_rear` (rear view) according to `interval` period
- **common_component**: Common component example, receiving three messages at the same time: `/topic/chatter` (String), `/topic/image_front` (Image), `/topic/image_rear` (Image), triggered by **message** (Proc is triggered when there is a new message in the first channel)
- **sync_component**: Example of synchronization component, **multiple messages are synchronized according to time window** and trigger Proc, receiving three channels: `/topic/image_front` (REQUIRED), `/topic/image_rear` (WAITABLE), `/topic/chatter` (OPTIONAL)

**Key Features**:

- `TimerComponent` inherits from the timer component to implement periodic publishing tasks
- `Component` supports multiple message type subscriptions. The message type is specified through template parameters. The triggering time is "the first reader has new messages"
- `SyncComponent` supports multi-channel message time alignment: each channel can be configured as REQUIRED (required to match), WAITABLE (required for real-time, optional for timeout), OPTIONAL (optional), and used with `accept_diff`, `main_msg_cycle`, `accept_latency` and other parameters for synchronization matching and timeout compensation
- Component development facilitates modular management

**Operation mode**:

- **timer** only: `component_example/timer_component/scripts/launch.sh` (send forward/backward view)
- **common** only: `component_example/common_component/scripts/launch.sh` (receiving three routes, other nodes need to send chatter/image_front/image_rear)
- **timer + sync** (recommended joint debugging): `component_example/sync_component/scripts/launch.sh`, load `sync.dag`, and start timer (send forward view/back view) and sync_component (receive three channels, chatter is OPTIONAL and optional) at the same time.

**Note**: If common_component / sync_component requires `/topic/chatter`, another node (such as topic_talker) needs to publish String; when only running sync.dag, the timer only sends two images, and chatter is optional.

---

### 6. concurrent_example - Concurrency infrastructure example

Demonstrates the use of concurrency infrastructure in the Segar framework:

- **tasker**: Comprehensive example showing the use of all concurrency primitives

**Key Features**:

- **Async**: Execute tasks asynchronously and get results (via `future`)
- **Execute**: fire-and-forget task execution, without waiting for the result
- **TaskEvent**: event synchronization mechanism, used for coordination between tasks
- **LockGuard**: Coroutine-safe mutex protection to protect shared resources
- **Yield**: Give up the execution rights of the coroutine to prevent starvation caused by long-term occupation
- **SleepFor**: The coroutine sleeps safely and correctly gives up execution rights in the coroutine environment.

**Example scenario**:

- Use `Async` to start multi-stage tasks and wait for completion through `future`
- Use `Execute` to start background tasks (fire-and-forget)
- Use `TaskEvent` to implement event notification
- Use `LockGuard` to protect shared data structures
- Use `Yield` in loop calculations to prevent coroutine starvation

---

### 7. zero_copy_example - Topic Zero Copy example

Demonstrate the basic usage of Topic zero-copy in the Segar framework:

- **zero_copy_talker**: Sender sample, use `LoanedMessage<Image>` and `LoanSample()` to apply for loaned sample, and publish it to `/topic/zero_copy`
- **zero_copy_listener**: Receiver example, subscribe to `/topic/zero_copy`, read the underlying `Image` message in `LoanedMessage<Image>`

**Key Features**:

- Use `LoanedMessage<example::msg::Image>` as Topic message type
- Use `CreateWriter<LoanedMessage<...>>` to create a zero-copy Writer
- Use `LoanSample()` to get loaned sample
- Use `Write(loaned_sample)` to send zero-copy message
- The receiving end accesses the underlying original message through `msg->GetMessage()`

**Description**:

- This example directly reuses the existing `example/msg/Image.msg`
- This means that zero-copy is a usage of Topic, and there is no need to specifically define a new message type for zero-copy, but the message type must meet the Plain standard or `IsBoundedMessage<T>`.

Each example directory contains: **source code** `src/`, **configuration** `config/`, **launch script** `scripts/launch.sh`.
