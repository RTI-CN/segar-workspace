# Segar example description

The document includes compilation instructions, running instructions and introduction to each example.

---

## Compilation environment instructions

Please use this repository on ubuntu system (ubuntu 22.0.4 recommended).
The build environment requires CMake 3.10+, C++17. Please make sure pip3 is installed (can be installed using sudo apt install python3-pip).
x86 and Orin use different compilers and scripts.

### x86 compilation

- **Compiler**: GCC 9.5.0 (x86_64-linux-gnu)
- **Step**: Execute in the workspace root directory:

```bash
./scripts/build_x86.sh
```

- **Optional parameters**: `-d` compiles Debug; `-r` cleans the build directory; `-ra` cleans build and `install/x86_64` at the same time
- **Product Catalog**: `build_x86/output/`
- **Packaging**: `./scripts/pkg_x86.sh`, package `build_x86/output/` into tgz, and store it in the `build_x86` directory

### Orin cross compilation

- **Compiler**: GCC 13.2.0 (aarch64-none-linux-gnu, for Orin/ARM64)
- **Toolchain**: needs to be deployed to the `/opt` directory, and ultimately needs to exist in `/opt/x-tools/aarch64-none-linux-gnu`
- **Download**: <https://developer.nvidia.com/downloads/embedded/L4T/r38_Release_v2.0/release/x-tools.tbz2>
- **Unzip** (root required): `sudo tar -xjf x-tools.tbz2 -C /opt`
- **Step**: Execute in the workspace root directory:

```bash
./scripts/build_orin.sh
```

- **Optional parameters**: `-d` compiles Debug; `-r` cleans the build directory; `-ra` cleans build and `install/orin` at the same time
- **Product Catalog**: `build_orin/output/`
- **Packaging**: `./scripts/pkg_orin.sh`, package `build_orin/output/` into tgz, and store it in the `build_orin` directory

---

## Operating Instructions

### Example run

The executable file is located in the `output` directory of each platform, for example: `build_x86/output/<example_name>/<target_name>/` or `build_orin/output/...`.

- **Single example**: Enter the corresponding example directory and execute its `scripts/launch.sh`:

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

`segar` command line tool is used to view/operate nodes, Topics, services, Actions, parameters, bags, etc. You need to start the example or `start_all.sh` first, and then execute it in the same output directory:

```bash
./scripts/run_segar_cli.sh
```

The script will execute `segar param`, `segar node`, `segar topic`, `segar service`, `segar action`, `segar bag` and other subcommands in sequence, and the log will be written under `logs/`.

**Run a single segar command**: After executing `source segar_setup.bash` in the output directory to import the environment variables, you can manually execute commands such as `segar param` separately.

**Run tracing**: After executing `source segar_setup.bash` in the output directory to import the environment variables, execute `mainboard -d config/tracing_node.dag` to start tracing data collection. First time use requires `sudo tracing -i` to initialize MySQL; for import and query, see [Tracing User Guide](Segar_Tracing.md).

---

## Example list

### 1. Topic_example - Topic publish and subscribe example

Demonstrates the publish and subscribe function of Topic in the Segar framework:

- **Topic_talker**: Publisher example, use `Timer` to periodically publish messages to the `/topic/chatter` Topic
- **Topic_listener**: Subscriber example, subscribe to the `/topic/chatter` Topic and receive messages

**Key Features**:

- Create a publisher using `CreateWriter`
- Create a subscriber using `CreateReader`
- Use `Timer` to implement periodic releases

---

### 2. service_example - Service service call example

Demonstrates the request response function of Service in the Segar framework:

- **service_server**: Server example, providing `set_camera_info` service
- **service_client_sync**: synchronous client example, using synchronous method to call services
- **service_client_async**: Asynchronous client example, calling services asynchronously

**Key Features**:

- Use `CreateService` to create a server
- Create a client using `CreateClient`
- Synchronous call: `SyncSendRequest`
- Asynchronous call: `SendRequest` with callback function
- Use `Timer` to implement periodic requests

---

### 3. param_example - Parameter parameter management example

Demonstrates the parameter management function of Parameter in the Segar framework:

- **param_server**: parameter server example, using the local parameter interface (Local Parameter API)
- **param_client**: Parameter client example, using Remote Parameter API

**Key Features**:

- Local parameter operation: `Segar_Set_Local_Param`, `Segar_Get_Local_Param`, `Segar_List_Local_Params`, `Segar_Dump_Local_Params`, `Segar_Load_Local_Params`
- Remote parameter operation: `Segar_Get_Remote_Param`, `Segar_Set_Remote_Param`, `Segar_List_Remote_Params`, `Segar_Dump_Remote_Params`, `Segar_Load_Remote_Params`
- Supports basic types (int, string, etc.) and Protobuf message types
- Define the parameter structure using a custom proto file

---

### 4. Action_example - Action Action execution example

Demonstrates the long-term task execution function of Action in the Segar framework:

- **Action_server**: Action server example, executing `lookup_transform` Action
- **Action_client_sync**: Synchronous client example, using synchronous method to send the target and wait for the result
- **Action_client_async**: Asynchronous client example, uses asynchronous method to send targets, and receives feedback and results through callbacks

**Key Features**:

- Create an Action server using `CreateActionServer`
- Create an Action client using `CreateActionClient`
- Support the complete life cycle of Goal, Feedback and Result
- Support target cancellation (Cancel) function
- Synchronous call: `SyncSendGoal` + `WaitForResult`
- Asynchronous call: `AsyncSendGoal` + callback function
- Use `Timer` to achieve periodic sending targets

---

### 5. component_example - Component component example

Demonstrates the component development method of Component in the Segar framework:

- **timer_component**: Timer component example, periodically publishes `Image` messages to `/topic/image` Topic
- **common_component**: Common component example, receiving two types of messages at the same time: `Image` and `String`

**Key Features**:

- `TimerComponent` inherits from the timer component to implement periodic release tasks
- `Component` supports multiple message type subscriptions, and the message type is specified through template parameters.
- Component development facilitates modular management

**Note**: Since `common_component` needs to receive `String` type messages, `topic_talker` needs to be run at the same time during testing to publish `String` messages.

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

- Use `Async` to start multi-stage tasks and wait for completion with `future`
- Use `Execute` to start background tasks (fire-and-forget)
- Use `TaskEvent` to implement event notification
- Protect shared data structures using `LockGuard`
- Use `Yield` in loop calculations to prevent coroutine starvation

---

Each example directory contains: **source code** `src/`, **configuration** `config/`, **startup script** `scripts/launch.sh`.
