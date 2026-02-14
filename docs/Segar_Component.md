# Getting Started with Segar Component

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> Component is the basic execution unit of the Segar system. It relies on DAG file configuration to start. It is divided into two models: **message trigger** and **timer trigger**.
>
> **Quick process**: Write DAG configuration file → Write Component business code → Compile → Mainboard starts the component → CLI to verify the running status

---

## 1. Quick overview of core concepts

- **Component**: Business function carrier, including Init (initialization) and Proc (data processing) core logic, divided into two categories: message triggering and timer triggering
- **.idl | .msg | .proto**: Protocol file that defines the communication data structure. C++ code needs to be generated through tools
- **DAG file**: component startup configuration file (Proto text format), defining component dependencies, input Topics, parameter paths, etc.
- **mainboard**: Segar framework launcher, which starts components by loading DAG files and supports extended parameters such as process scheduling and plug-in loading.
- **SEGAR_REGISTER_COMPONENT**: Registration macro, must be used to expose component classes, otherwise the framework cannot recognize it
---

## 2. Message trigger model

Define the list of Topics that need to be received in the DAG file. When a new message arrives for the first Topic defined in the DAG file, the latest messages of all Topics are taken as input parameters to trigger the Proc function.

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
# When a new message arrives in the Topic of the first readers, trigger Proc
      readers {
        topic: "/topic/image"
        pending_queue_size: 5
      }
      readers {
        topic: "/topic/chatter"
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
- **(required)** Add `SEGAR_REGISTER_COMPONENT(ComponentClassName)` registration macro at the end

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

class CommonComponentExample : public rti::segar::Component<Image, String> {
 public:
  bool Init() final;
  bool Proc(const std::shared_ptr<Image>& msg0,
            const std::shared_ptr<String>& msg1) final;
};
SEGAR_REGISTER_COMPONENT(CommonComponentExample)
```

**Implementation File** (`src/component_example/common_component/src/common_component_example.cc`):

```cpp
#include "common_component_example.h"

bool CommonComponentExample::Init() {
  AINFO << "CommonComponentExample init";
  return true;
}

bool CommonComponentExample::Proc(const std::shared_ptr<Image>& msg0,
                                  const std::shared_ptr<String>& msg1) {
  AINFO << "Start common component Proc [msg0->width:" << msg0->width()
        << "] [msg1->data:" << msg1->data() << "]";
  return true;
}
```

---

## 3. Timer trigger model

Trigger the Proc function periodically at `interval` (milliseconds) configured in the DAG.

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
- **(required)** Override `Init()` method (initialize Writer/parameters, etc.)
- **(required)** Override the `Proc()` method (business logic executed regularly according to interval)
- **(required)** Add `SEGAR_REGISTER_COMPONENT(ComponentClassName)` registration macro at the end

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

**Implementation File** (`src/component_example/timer_component/src/timer_component_example.cc`):

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

## 4. Use `flag_file_path` and `config_file_path` to complete process-level/dag-level personalized running context configuration
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

### 4.1 Specific usage examples of `flag_file_path`

`flag/my_component.flag`:

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

### 4.2 Specific reading example of `config_file_path`

Suppose you have configuration message `MyComponentConfig` (`my_component.pb.txt` corresponds to it):

```proto
message MyComponentConfig {
  optional string input_topic = 1;
  optional uint32 queue_size = 2;
}
```

`conf/my_component.pb.txt`:

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
```
---

## 5. Run

mainboard is the entry point for starting Components. Common startup patterns are:

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

## 6. (Optional reading) Advanced instructions

### 6.1 Precautions for using parameters

- Multiple DAG priority: Multiple DAG files specified by `-d` are loaded in order, and components with the same name are subject to the last loaded DAG configuration.

### 6.2 Logs and troubleshooting

- **Log Level**: `export SEGAR_LOG_LEVEL=DEBUG && mainboard -d config/common.dag`
- **Common Mistakes**:
  - DAG file path error: log prompt `Cannot find dag_conf_file`, check `-d` parameter path
  - Component class name does not match: log prompt `Component [XXX] not found`, check that `component_class_name` in DAG is consistent with the code class name
