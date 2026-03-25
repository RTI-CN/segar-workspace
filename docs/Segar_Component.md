# Getting started with Segar Component

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> Component is the basic execution unit of the Segar system. It relies on DAG file configuration to start. It is divided into two models: **message trigger** and **timer trigger**.
>
> **Quick process**: Write DAG configuration file → Write Component business code → Compile → Mainboard starts the component → CLI to verify the running status

---

## 1. Quick overview of core concepts

- **Component**: Business function carrier, including Init (initialization) and Proc (data processing) core logic, divided into two categories: message triggering and timer triggering
- **.idl | .msg | .proto**: Protocol file that defines the communication data structure. C++ code needs to be generated through tools
- **DAG file**: component startup configuration file (Proto text format), defining component dependencies, input topics, parameter paths, etc.
- **mainboard**: Segar framework launcher, which starts components by loading DAG files and supports extended parameters such as process scheduling and plug-in loading.
- **SEGAR_REGISTER_COMPONENT**: Registration macro, must be used to expose component classes, otherwise the framework cannot recognize it
---

## 2. Message trigger model

Define the list of topics that need to be received in the DAG file. When a new message arrives for the first Topic defined in the DAG file, the latest messages of all topics are taken as input parameters to trigger the Proc function.

### 2.1 DAG configuration

- File location: It is recommended to place it in the project `config/` directory
- Use the `components` node to configure `readers` to specify the subscribed Topic

Example `common.dag` (`src/component_example/common_component/config/`):

```text
# Define all components in the DAG stream (Proto text format)
module_config {
#Required: component dynamic library path (relative path "relative to the path of the currently executing command"/absolute path can be used)
  module_library: "lib/libcommon_component.so"

# Component list: A module can contain multiple components
  components {
# Required: component class name (exactly the same as the custom component class name in the code)
    component_class_name: "CommonComponentExample"

# Component-specific configuration
    config {
# Required: component internal node name (custom unique identifier, used for binding resources)
      inner_node_name: "common_component_example"

# Optional: parameter file path (leave blank if no parameters are required)
      params_file_path: "config/params.yaml"

# Optional: Enter Topic configuration (multiple inputs correspond to Proc function parameters in order)
# When a new message arrives in the Topic of the first reader, trigger Proc
      readers {
        topic: "/topic/chatter"
        pending_queue_size: 5
      }
      readers {
        topic: "/topic/image_front"
        pending_queue_size: 5
      }
      readers {
        topic: "/topic/image_rear"
        pending_queue_size: 5
      }
    }
  }
}
```
### 2.2 C++ implementation

- **(required)** Inherits the `Component<InputType1, InputType2, ...>` base class, and the template parameters are input data types
- **(required)** Override the `Init()` method (executed once when the component starts)
- **(required)** Override the `Proc()` method (triggered when a new message arrives in the first Topic, core business logic)
- **(required)** Add `SEGAR_REGISTER_COMPONENT(component class name)` registration macro at the end

**Header file** (`src/component_example/common_component/src/common_component_example.h`):

```cpp
/**
* Message triggering component: Template parameter order/type must be exactly the same as the readers configuration in the DAG file
* Trigger timing: When a new message arrives in the Topic of the first reader in the DAG, Proc is called
 */
#include "example/msg/Image.hpp"
#include "example/msg/String.hpp"

#include "segar/component/component.h"
using example::msg::Image;
using example::msg::String;

class CommonComponentExample : public rti::segar::Component<String, Image, Image> {
 public:
  bool Init() final;
  bool Proc(const std::shared_ptr<String>& msg0,
            const std::shared_ptr<Image>& msg1,
            const std::shared_ptr<Image>& msg2) final;
};
SEGAR_REGISTER_COMPONENT(CommonComponentExample)
```
**Implementation file** (`src/component_example/common_component/src/common_component_example.cc`):

```cpp
#include "common_component_example.h"

bool CommonComponentExample::Init() {
  AINFO << "CommonComponentExample init";
  return true;
}

bool CommonComponentExample::Proc(const std::shared_ptr<String>& msg0,
                                  const std::shared_ptr<Image>& msg1,
                                  const std::shared_ptr<Image>& msg2) {
  uint32_t w1 = 0, w2 = 0;
  if (msg1) w1 = msg1->width();
  if (msg2) w2 = msg2->width();
  AINFO << "CommonComponentExample Proc [chatter:" << (msg0 ? msg0->data() : "")
        << "] [image_front->width:" << w1 << "] [image_rear->width:" << w2 << "]";
  return true;
}
```
---

## 3. Timer trigger model

Trigger the Proc function periodically at the `interval` (milliseconds) configured in the DAG.

### 3.1 DAG configuration

- Use `timer_components` node (different from `components` of ordinary components)
- `interval` must be configured to specify the execution interval (unit: milliseconds)

Example `timer.dag` (`src/component_example/timer_component/config/`):

```text
# Define the timer component in the DAG stream (Proto text format)
module_config {
  module_library: "lib/libtimer_component.so"

# Timer component exclusive configuration node
  timer_components {
    component_class_name: "TimerComponentExample"

    config {
      inner_node_name: "timer_component_example"
# Required: execution interval (unit: milliseconds), the Proc function is called every 100ms
      interval: 100
    }
  }
}
```
### 3.2 C++ implementation

- **(required)** Inherits the `TimerComponent` base class (no template parameters required)
- **(required)** Override the `Init()` method (initialize Writer/parameters, etc.)
- **(required)** Override the `Proc()` method (business logic executed regularly according to interval)
- **(required)** Add `SEGAR_REGISTER_COMPONENT(component class name)` registration macro at the end

**Header file** (`src/component_example/timer_component/src/timer_component_example.h`):

```cpp
/**
* Timer trigger component: execute Proc periodically according to the interval in DAG
 */
#include "example/msg/Image.hpp"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/component/timer_component.h"

class TimerComponentExample : public rti::segar::TimerComponent {
 public:
  bool Init() final;
  bool Proc() final;

 private:
  using ImageWriter = rti::segar::Writer<example::msg::Image>;
  std::shared_ptr<ImageWriter> image_writer_ = nullptr;
  uint32_t proc_count_ = 0;
};
SEGAR_REGISTER_COMPONENT(TimerComponentExample)
```
**Implementation file** (`src/component_example/timer_component/src/timer_component_example.cc`):

```cpp
#include "timer_component_example.h"

bool TimerComponentExample::Init() {
  image_writer_ = node_->CreateWriter<example::msg::Image>("/topic/image");
  RETURN_VAL_IF(!image_writer_, false);
  return true;
}

bool TimerComponentExample::Proc() {
  auto out_msg = std::make_shared<example::msg::Image>();
  out_msg->width(proc_count_++);
  AINFO_IF(!image_writer_->Write(out_msg))
      << "Failed to write msg:" << out_msg->width();
  AINFO << "timer_component_example: Write image msg->width:"
        << out_msg->width();
  return true;
}
```
**Scheduling configuration**: Components can specify CPU affinity, scheduling strategy, etc. through the scheduling configuration file, and the mainboard is loaded through the `-s` parameter. For details, see [Getting Started with Scheduler](Segar_Scheduler.md).

---

## 4. Event-triggered message synchronization model (SyncComponent)

Multiple messages are aligned according to the time window and trigger a Proc, which is suitable for scenarios such as multi-sensor/multi-source data fusion. Different from the "triggered when the first path arrives" of ordinary Component, SyncComponent only calls Proc after **Time Window Match** or **Timeout Compensation Match** is successful.

### 4.1 Code interface

- **(required)** Inherit `SyncComponent<M0, M1, ...>`, the order of template parameters is consistent with the order of `readers` in DAG
- **(required)** Rewrite `Proc(const std::shared_ptr<M0>&, const std::shared_ptr<M1>&, ...)`: called when synchronization is successful, and the input parameter is the message after the alignment of all parties
- **(required)** Override `GetTimeStamp(size_t index, const std::shared_ptr<M0>&, ...)`: Return the timestamp of the `index` message (for within-window matching); returning 0 will cause the matching to fail.
- **(optional)** Rewrite `TimeoutProc()`: triggered when no matching is successful in this cycle, used for alarm or compensation logic; default empty implementation. Be careful not to do time-consuming operations internally, otherwise the next cycle timeout will be latencyed.

### 4.2 Message synchronization type (type of readers)

| type | meaning | real-time matching | timeout compensation matching |
|-----------|----------|--------------|----------------|
| REQUIRED | Hard constraint | Must match | Must match |
| WAITABLE | Waiting type | Participate as required | Can be missing |
| OPTIONAL | Optional | Does not trigger matching | Can be missing |

- **REQUIRED**: The time window must be met in both real-time matching and timeout compensation, otherwise this frame will not be delivered
- **WAITABLE**: participates in the same way as REQUIRED during real-time matching; missing is allowed during timeout compensation. Only when WAITABLE exists in the configuration, compensation matching (FlushKeyFrames) will be performed in the timeout callback.
- **OPTIONAL**: When a message arrives, it will only be queued and will not actively trigger matching; if it is missing, it will not block delivery.

### 4.3 DAG configuration

Use the `sync_components` node (different from `components` / `timer_components`) to configure `SyncComponentConfig`:

- **inner_node_name**: component internal node name (required)
- **accept_diff**: time window, unit millisecond (relative to the allowed time difference of the base message, that is \|t_msg - t_base\| ≤ accept_diff)
- **main_msg_cycle**: Main cycle, in milliseconds (expected main frame cycle)
- **accept_latency**: additional waiting grace after the main cycle, in milliseconds
- **readers**: one `topic`, `type` (REQUIRED/WAITABLE/OPTIONAL), `buffer_size` for each channel

Example (`src/component_example/sync_component/config/sync.dag`):

```text
module_config {
  module_library: "lib/libsync_component.so"
  sync_components {
    component_class_name: "SyncComponentExample"
    config {
      inner_node_name: "sync_component_example"
      accept_diff: 2250
      main_msg_cycle: 5000
      accept_latency: 2450
      readers { topic: "/topic/image_front" type: REQUIRED buffer_size: 5 }
      readers { topic: "/topic/image_rear"  type: WAITABLE buffer_size: 5 }
      readers { topic: "/topic/chatter"     type: OPTIONAL buffer_size: 5 }
    }
  }
}
```
In practice, it is recommended to assign the most stable path to the first reader (REQUIRED) as the main path.

### 4.4 Brief description of triggering and matching behavior

1. Any reader joins the queue first after receiving the message; if the path is OPTIONAL, matching will not be triggered this time.
2. A **real-time matching** is triggered when the REQUIRED/WAITABLE message arrives: using the current path as the base, match other paths within the `accept_diff` time window; if OPTIONAL is missing, it will be allowed, but if REQUIRED/WAITABLE is missing, this matching will fail.
3. Initialization starts the periodic timer; **Only when WAITABLE** exists in the configuration, **compensation matching** will be done in the timeout callback (trying from new to old using the latest message as the base). REQUIRED is still a hard constraint in the compensation path, and WAITABLE/OPTIONAL is optional.
4. If there is no successful match in this cycle, `TimeoutProc()` will be called first, and then the timer will be restarted.

### 4.5 Parameter tuning suggestions

- **main_msg_cycle**: estimated based on the main channel frame rate, such as T = 1000/fps (ms), the initial value can be round(T)
- **accept_diff**: 0.2T~0.4T when the timestamp quality is good, 0.4T~0.7T when the cross-process/cross-machine jitter is large
- **accept_latency**: 0.2T～0.4T without WAITABLE, 0.5T～1.0T with WAITABLE

Recommended initial values for common frame rates (unit: milliseconds):

| Main channel frame rate | main_msg_cycle | accept_diff | accept_latency (without wait) | accept_latency (with wait) |
|----------|----------------|-------------|----------------------------------|----------------------------------|
| 10 FPS | 100 | 20～40 | 20～40 | 50～100 |
| 20 FPS | 50 | 10～20 | 10～20 | 25～50 |
| 30 FPS | 33 | 7～13 | 7～13 | 16～33 |
| 60 FPS | 17 | 3～7 | 3～7 | 8～17 |

When adjusting parameters, it is recommended to only change one parameter at a time and fix `main_msg_cycle` first; if TimeoutProc is frequent and the downstream can accept later output, you can increase `accept_latency` first; if the time difference between the matched frames is too large, you can reduce `accept_diff`. See `example/sync_component requirements and usage.md` for more details.

---

## 5. Use `flag_file_path` and `config_file_path` to complete process-level/dag-level personalized running context configuration

```xml
module_config {
  module_library : "lib/libmy_component.so"
  components {
    component_class_name : "MyComponent"
    config {
      inner_node_name : "my_node"
      config_file_path : "conf/my_component.pb.txt"
      flag_file_path : "flag/my_component.flag"
    }
  }
}
```
### 5.1 Specific usage examples of `flag_file_path`

`flag/my_component.flag`：

```bash
--my_enable_feature=true
--my_timeout_ms=300
```
Component code (only the key parts are shown):

```cpp
#include "gflags/gflags.h"

DEFINE_bool(my_enable_feature, false, "enable feature");
DEFINE_int32(my_timeout_ms, 100, "timeout ms");

bool MyComponent::Init() {
  AINFO << "enable_feature=" << FLAGS_my_enable_feature
        << ", timeout_ms=" << FLAGS_my_timeout_ms;
  return true;
}
```
### 5.2 Specific reading example of `config_file_path`

Assume you have the configuration message `MyComponentConfig` (`my_component.pb.txt` corresponds to it):

```proto
message MyComponentConfig {
  optional string input_topic = 1;
  optional uint32 queue_size = 2;
}
```

`conf/my_component.pb.txt`：

```textproto
input_topic: "NodeA"
queue_size: 128
```
Component code reads:

```cpp
bool MyComponent::Init() {
  MyComponentConfig cfg;
  if (!GetProtoConfig(&cfg)) {
    AERROR << "load my_component config failed, path=" << ConfigFilePath();
    return false;
  }
  AINFO << "input_topic=" << cfg.input_topic()
        << ", queue_size=" << cfg.queue_size();
  return true;
}
```---

## 6. Run

mainboard is the startup entrance of Component, which is started in the following ways:

- **--dag_conf=CONFIG_FILE** (`-d`): Load DAG configuration file (**required**), you can specify multiple times, such as `-d dag1.dag -d dag2.dag`

```bash
# Basic startup (single DAG)
mainboard -d config/common.dag

# Multiple DAG startup
mainboard -d config/common.dag -d config/timer.dag

#Specify process namespace and scheduling policy
mainboard -d config/car_component.dag -p car_component_proc
```
---

## 7. (Optional reading) Advanced instructions

### 7.1 Precautions for using parameters

- Multiple DAG priority: Multiple DAG files specified by `-d` are loaded in order. The component with the same name shall be subject to the last loaded DAG configuration.

### 7.2 Logs and Troubleshooting

- **Log level**: `export SEGAR_LOG_LEVEL=DEBUG && mainboard -d config/common.dag`
- **Common Mistakes**:
  - DAG file path error: log prompt `Cannot find dag_conf_file`, check `-d` parameter path
  - The component class name does not match: the log prompts `Component [XXX] not found`, check that the `component_class_name` in the DAG is consistent with the code class name
