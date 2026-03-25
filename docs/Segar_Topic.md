# Topic Getting Started

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> Topic messages are defined in ROS 2-style `.msg` files.
>
> **Quick process**: Write `.msg` → Write business code → Compile → Run Writer/Reader examples on both ends → CLI verification.

---

## 1. How to write .msg file

### 1.1 File Location / Naming Convention / Type / Namespace

File naming convention follows ROS 2 definition. For example, define a `String` message with namespace `example`:

```text
src/type_src/example/msg/String.msg
```
- Use CamelCase for filenames (e.g. `String.msg`)
- The name part of the `.msg` file is the Segar Message type name
- The directory of the `.msg` file is the namespace, for example `String` corresponds to `namespace example::msg;`
- The syntax and subsequent usage are the same as ROS 2 and will not be introduced further.

---

## 2. Basic usage examples of C++ (non-Component usage)

`src/topic_example/topic_talker/src/topic_talker.cc` and `src/topic_example/topic_listener/src/topic_listener.cc` demonstrate how to publish/subscribe `String` messages defined in `src/type_src/example/msg/String.msg`.

### 2.1 Code description

- **(required)** Contains automatically generated hpp header files
- **(required)** The `topic_name` of both the sender and the receiver must be exactly the same (for example: `/topic/chatter`)
- **(required)** `rti::segar::Init(argv[0]);` is used to initialize Segar system functions
- **(required)** `CreateNode` creates a node (node) that carries user services. The node can create Writer/Reader/Service/Client/ActionServer/ActionClient/ParameterServer/ParameterClient
- **(optional)** `rti::segar::WaitForShutdown();` is used to prevent the system main thread from exiting. Use Ctrl+C to exit.

### 2.2 Sender (Writer, pure asynchronous)

#### CreateWriter function description

- **Template parameter**: Message type defined in `.msg` file
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
### 2.3 Receiver (Reader, pure asynchronous)

#### CreateReader function description

- **Template parameter**: Message type defined in `.msg` file
- **Parameter 1**: Reader attribute (topic name, `pending_queue_size` The default number of messages cached for callback is 5, please fill in 1 for media data with high real-time requirements. For qos details, see `config/segar.pb.conf`)
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

## 3. Topic Zero Copy usage

When the Topic message meets the bounded condition, you can use `LoanedMessage` and `LoanSample` to send it in zero-copy mode to avoid data copying and serialization/deserialization at both ends, and improve the sending and receiving performance.

The corresponding example in the workspace is located at:

- `src/zero_copy_example/zero_copy_talker/src/zero_copy_talker.cc`
- `src/zero_copy_example/zero_copy_listener/src/zero_copy_listener.cc`

The current example directly reuses the existing `src/type_src/example/msg/Image.msg`, which means that zero-copy is a **send/receive method** for Topic and does not require the user to define an additional dedicated message type.

### 3.1 Sender (LoanedMessage Writer)

The main differences from ordinary Topic Writer are:

- Writer template parameter changed to `LoanedMessage<original message type>`
- No more `std::make_shared<Message>()`
- Instead call `LoanSample()` first, then fill in the message content, and finally `Write(loaned_sample)`

### 3.2 Receiver (LoanedMessage Reader)

The receiver template parameter also uses `LoanedMessage<original message type>`.
In the callback, first obtain the underlying original message through `msg->GetMessage()`, and then read the fields according to the normal message method.

---

## 4. CLI Debugging

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

## 5. Topic custom shm allocation strategy
### 5.1 Support customizing shm allocation strategies for specific topics
- This configuration file is customized for topics that have special requirements for shm resource usage/performance requirements;
- The default adaptive policy of the original topic without this configuration will continue to take effect;
### 5.2 Global conf/topics.pb.conf file
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

### 5.3 Configuration item description
- topic: topic name;
- enable: Whether the customized configuration for this Channel takes effect.
  -Default value (value when not configured): true
  - Optional values: true, false
- block_num: The number of blocks in the current Channel. After setting this value, the channel will no longer use the system's default calculation strategy;
  -Default value (value when not configured): 1
  - Optional value: any integer greater than 1, if set to 0 it will be automatically modified to 1
