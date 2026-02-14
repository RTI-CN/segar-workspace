# Getting Started with Topics

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> Topic messages are defined in ROS 2-style `.msg` files.
>
> **Quick process**: Write `.msg` → Write business code → Compile → Run Writer/Reader examples on both ends → CLI verification.

---

## 1. How to write .msg file

### 1.1 File Location/Naming Convention/Type/Namespace

File naming convention follows ROS 2 definition. For example, define a `String` message with namespace `example`:

```text
src/type_src/example/msg/String.msg
```

- Use CamelCase for filenames (e.g. `String.msg`)
- The name part of the `.msg` file is the Segar Message type name
- The directory of the `.msg` file is the namespace, for example, `String` corresponds to `namespace example::msg;`
- The syntax and subsequent usage are the same as ROS 2 and will not be introduced further.

---

## 2. Basic usage examples of C++ (without Components)

`src/topic_example/topic_talker/src/topic_talker.cc` and `src/topic_example/topic_listener/src/topic_listener.cc` demonstrate how to publish/subscribe `String` messages defined by `src/type_src/example/msg/String.msg`.

### 2.1 Code description

- **(required)** Contains automatically generated hpp header files
- **(required)** `topic_name` of both sender and receiver must be exactly the same (example: `/topic/chatter`)
- **(required)** `rti::segar::Init(argv[0]);` is used to initialize Segar system functions
- **(required)** `CreateNode` creates a node (node) that carries user services. The node can create Writer/Reader/Service/Client/ActionServer/ActionClient/ParameterServer/ParameterClient
- **(optional)** `rti::segar::WaitForShutdown();` is used to prevent the system main thread from exiting. Use Ctrl+C to exit.

### 2.2 Sender (Writer, fully asynchronous)

#### CreateWriter function description

- **Template parameter**: Message type defined by `.msg` file
- **Parameter**: Topic name
- **Return value**: Writer used to send the message

#### Sender example

```cpp
#include <memory>
#include <string>

#include "example/msg/String.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);
  auto node = rti::segar::CreateNode("topic_talker");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto writer = node->CreateWriter<example::msg::String>("/topic/chatter");
  RETURN_VAL_IF(!writer, EXIT_FAILURE);
  uint32_t seq = 0;
  auto callback = [&writer, &seq]() {
    auto msg = std::make_shared<example::msg::String>();
    msg->data(std::to_string(seq++));
    AINFO_IF(!writer->Write(msg)) << "Failed to write msg:" << msg->data();
    AINFO << "Sent message: " << msg->data();
  };
  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

### 2.3 Receiver (Reader, fully asynchronous)

#### CreateReader function description

- **Template parameter**: Message type defined by `.msg` file
- **Parameter 1**: Reader attribute (topic name, `pending_queue_size` defaults to 5 for the number of messages cached by the callback. For media data with high real-time requirements, please fill in 1. For QoS details, see `config/segar.pb.conf`)
- **Parameter 2**: callback for processing messages received from the Writer side
- **Return value**: Reader object

#### Receiver example

```cpp
#include <memory>
#include <string>

#include "example/msg/String.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("topic_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto reader = node->CreateReader<example::msg::String>(
      "/topic/chatter",
      [](const auto& msg) { AINFO << "Received message: " << msg->data(); });
  RETURN_VAL_IF(!reader, EXIT_FAILURE);
  AINFO << "Waiting for messages...";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

---

## 3. CLI Debugging

The `segar topic` command can be used to query Topic (for detailed functions, please refer to the Segar CLI Getting Started Document):

```bash
$ segar topic
usage: segar topic [-h] {list,info,bw,hz,type,echo} ...

Various topic related utilities

commands:
  list                        List all topics
  info <TopicName>            Print information about a topic
  bw <TopicName>              Display bandwidth of a topic
  hz <TopicName>              Display publishing rate of topic
  type <TopicName>            Display type of topic
  echo <TopicName>            Echo messages from a topic (protobuf only)
```

---

## 4. Topic custom shm allocation strategy
### 4.1 Support customizing shm allocation strategies for specific topics
- This configuration file is customized for topics that have special requirements for shm resource usage/performance requirements;
- The default adaptive policy of the original topic without this configuration will continue to take effect;
### 4.2 Global conf/topics.pb.conf file
topics: [
    {
        topic: "topic/discovery_0"
        block_num: 2
    }, {
        topic: "topic/discovery_1"
    }, {
        topic: "topic/discovery_2"
        block_num: 8
    }, {
        topic: "channel/discovery_3"
        enable: false
        block_num: 8
    }
]

### 4.3 Configuration item description
- topic: topic name;
- enable: Whether the customized configuration for this Channel takes effect.
  - Default value (value when not configured): true
  - Optional values: true, false
- block_num: The number of blocks in the current Channel. After setting this value, the channel will no longer use the system default calculation strategy;
  - Default value (value when not configured): 1
  - Optional value: any integer greater than 1, if set to 0 it will be automatically modified to 1


