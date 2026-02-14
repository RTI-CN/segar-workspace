# Getting Started with Action

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> The general basic knowledge of getting started with Segar Topic and getting started with Segar Service has been introduced and will not be repeated in this article.
>
> Action messages are defined in ROS 2-style `.action` files.
>
> **Quick process**: Write `.action` → Write business code → Compile → Run ActionServer/ActionClient examples on both ends → CLI verification.

---

## 1. How to write .action file

### 1.1 File Location/Naming Convention/Type/Namespace

File naming convention follows ROS 2 definition. For example, define a `LookUpTransform` action with namespace `example`:

```text
src/type_src/example/action/LookUpTransform.action
```

- Use CamelCase for filenames (e.g. `LookUpTransform.action`)
- The name part of the `.action` file is the Segar Action type name
- The directory of the `.action` file is the namespace, for example, `LookUpTransform` corresponds to `namespace example::action;`
- The syntax and usage are the same as ROS 2 and will not be introduced further.

---

## 2. Basic usage examples of C++ (without Components)

The example is excerpted from `src/action_example/*/action_*.cc`, split into two examples: Server side and Client side.

### 2.1 Code description

- **(required)** Contains automatically generated Action messages hpp file
- **(required)** `action_name` of both sender and receiver must be exactly the same (example: `lookup_transform`)
- **(required)** `rti::segar::Init(argv[0]);` is used to initialize Segar system functions
- **(optional)** `rti::segar::WaitForShutdown();` is used to prevent the system main thread from exiting. Use Ctrl+C to exit.

### 2.2 Server (Action Server, fully asynchronous)

#### ActionServer\<T\>::Callbacks member description

- **on_goal**: Process the goal request of ActionClient and return `false` to reject the goal
- **on_cancel**: Process the cancel request of ActionClient and return `false` to reject the user's cancel goal.
- **on_execute**: Process the goal message of ActionClient and decide how to respond as follows:
  1. `PublishFeedback(goal_id, feedback)`: Publish current progress
  2. `CancelGoal(goal_id, result)`: Cancel the goal midway and send the midway result to the Client
  3. `Succeed(goal_id, result)`: successfully processed the goal and sent the final result to the Client

#### CreateActionServer function description

- **Template parameters**: action type defined in `.action` file
- **Parameter 1**: action name
- **Parameter 2**: Callbacks of ActionServer
- **Parameter 3**: ActionOptions advanced properties (optional)
- **Return value**: ActionServer object

#### Server example

(`src/action_example/action_server/src/action_server.cc`)

```cpp
#include <atomic>
#include <memory>

#include "example/action/LookUpTransform.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using ActionServer =
      rti::segar::action::ActionServer<example::action::LookUpTransform>;
  using GoalID = rti::segar::action::GoalID;
  using LookUpTransform = example::action::LookUpTransform;

  std::shared_ptr<ActionServer> server;

  ActionServer::Callbacks callbacks;

  callbacks.on_goal = [](ActionServer& /*server*/, const GoalID& /*goal_id*/,
                         const LookUpTransform::Goal& goal) -> bool {
    AINFO << "Accepted goal: target_frame=" << goal.target_frame();
    return true;
  };

  callbacks.on_cancel = [](ActionServer& /*server*/,
                           const GoalID& goal_id) -> bool {
    AINFO << "Cancel request received for goal_id: "
          << rti::segar::action::internal::GoalIDToString(goal_id);
    return true;
  };

  callbacks.on_execute =
      [&server](ActionServer& /*server_ref*/, const GoalID& goal_id,
                const LookUpTransform::Goal& goal,
                const std::shared_ptr<std::atomic<bool>>& cancel_requested) {
        auto result =
            std::make_shared<example::action::LookUpTransform::Result>();

        const auto& target_frame = goal.target_frame();
        AINFO << "Executing target_frame: " << target_frame;

        constexpr int32_t steps = 5;
        for (int32_t i = 1; i <= steps; ++i) {
          AINFO << "Goal executing at step " << i << "/" << steps;

          auto feedback =
              std::make_shared<example::action::LookUpTransform::Feedback>();
          feedback->current(i);
          if (server) {
            server->PublishFeedback(goal_id, feedback);
          }

          if (cancel_requested->load()) {
            AINFO << "Goal cancelled at step " << i << "/" << steps;
            result->error(-1);
            result->transform("cancelled");
            if (server) {
              server->CancelGoal(goal_id, result);
            }
            return;
          }

          rti::segar::SleepFor(std::chrono::milliseconds(20));
        }

        result->transform("transform_from_" + target_frame);
        result->error(0);
        AINFO << "Goal completed successfully: target_frame=" << target_frame;
        if (server) {
          server->Succeed(goal_id, result);
        }
      };

  server =
      node->CreateActionServer<LookUpTransform>("lookup_transform", callbacks);
  RETURN_VAL_IF(!server, EXIT_FAILURE);

  AINFO << "Action server started successfully";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

#### Action Server Common ActionOptions Advanced Properties

(Only the commonly used ones are introduced, sorted by common priority)

- **max_execute_concurrency**: The number of tasks that Action Server can execute concurrently
- **max_active_goals**: The number of unfinished tasks that Action Server can cache
- **feedback_mode**: The mode for transmitting feedback to Action Client. If you want reliability, choose `OptionalMode::RTPS`; if you want efficiency, choose `OptionalMode::HYBRID`. Must be consistent with Action Client
- **status_publish_period_sec**: The interval for transmitting status to Action Client. The smaller the size, the higher the real-time performance; the larger the size, the lower the resource usage.
- **status_history_depth**: The cached history status size. If you need the full amount, increase the value. If you only need the latest one, set it to 1.

```cpp
rti::segar::action::ActionOptions action_options;
action_options.feedback_mode = OptionalMode::RTPS;
action_options.status_mode = OptionalMode::HYBRID;
action_options.get_result_qos = QosProfileConf::QOS_PROFILE_TF_STATIC;
action_options.status_publish_period_sec = 0.1;
action_options.terminal_state_retention_sec = 10.0;
action_options.max_active_goals = 256;
action_options.max_execute_concurrency = 8;

server = node->CreateActionServer<LookUpTransform>("lookup_transform", callbacks, action_options);
```

### 2.3 Client (Action Client, synchronous mode)

#### ActionClient\<T\>::GoalCallbacks member description

- **on_accept**: Handles the Ack Response received from ActionServer. `true`: ActionServer accepts the Goal request; `false`: ActionServer rejects the Goal request
- **on_result**: This function only takes effect in asynchronous mode and will be ignored in synchronous mode (using `SyncSendGoal`). Process the final Result received from ActionServer, and use the status parameter to obtain the Result in which of the following states: `STATUS_SUCCEEDED`, `STATUS_CANCELED`, `STATUS_ABORTED`
- **on_cancel**: Process the Cancel processing status result received from ActionServer (`ERROR_REJECTED`, `ERROR_UNKNOWN_GOAL`, `ERROR_TIMEOUT`, `ERROR_NOT_CANCELABLE`, `SUCCESS_REQUESTED`)
- **on_feedback**: Process Feedback messages received from ActionServer

#### CreateActionClient function description

- **Template parameters**: action type defined in `.action` file
- **Parameter 1**: action name
- **Parameter 2**: GoalCallbacks, subsequent `SendGoal`, `SyncSendGoal`, `CancelGoal` all use the callbacks defined by this parameter, and can be additionally covered on this basis
- **Parameter 3**: ActionOptions advanced properties (optional)
- **Return value**: ActionClient object

#### SyncSendGoal synchronization function description

- **Parameter 1**: Goal object to be sent
- **Parameter 2**: The GoalID that uniquely marks the Goal obtained after sending
- **Parameter 3**: GoalCallbacks (merged with global callbacks when created, the priority defined in this parameter is higher)
- **Return Value**: `true` means the Goal was successfully sent and the response was received and the ActionServer accepted the Goal; `false` means otherwise

#### WaitForResult synchronization function description

- **Parameter 1**: GoalID generated by `SyncSendGoal`
- **Parameter 2**: Object reference to store the final Result
- **Parameter 3**: Process the callback of Result, and obtain the Result status through the status parameter (`STATUS_SUCCEEDED`, `STATUS_CANCELED`, `STATUS_ABORTED`)
- **Return value**: `true` means that the final Result is obtained on time; `false` means that the Result is obtained after a timeout

#### Sync client example

(`src/action_example/action_client_sync/src/action_client_sync.cc`)

```cpp
#include <chrono>

#include "example/action/LookUpTransform.hpp"

#include "segar/segar.h"
namespace {
using ActionClient =
    rti::segar::action::ActionClient<example::action::LookUpTransform>;
using ActionClientSPtr = std::shared_ptr<ActionClient>;
using GoalID = rti::segar::action::GoalID;
using GoalStatusCode = rti::segar::action::GoalStatusCode;
using LookUpTransform = example::action::LookUpTransform;

void SyncCancelAfterSendGoal(ActionClientSPtr& client, const int32_t index) {
  LookUpTransform::Goal goal;
  goal.target_frame("map" + std::to_string(index));
  GoalID goal_id;
  if (!client->SyncSendGoal(goal, &goal_id)) {
    AERROR << "[Sync] SyncSendGoal failed at iteration " << index;
    return;
  }

  AINFO << "[Sync] Goal sent: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
// Wait for goal to actually start execution
  rti::segar::SleepFor(std::chrono::milliseconds(50));
  if (!client->SyncCancelGoal(goal_id)) {
    AERROR << "[Sync] CancelGoal failed, goal_id="
           << rti::segar::action::internal::GoalIDToString(goal_id);
    return;
  }
  AINFO << "[Sync] Cancel request sent: goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);

  if (!client->WaitForResult(goal_id)) {
    AERROR << "[Sync] cancel failed, goal_id="
           << rti::segar::action::internal::GoalIDToString(goal_id);
    return;
  }
  AINFO << "[Sync] cancel success: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
}
}  // namespace

void SyncSendGoal(ActionClientSPtr& client, const int32_t index) {
  LookUpTransform::Goal goal;
  goal.target_frame("map" + std::to_string(index));
  GoalID goal_id;
  if (!client->SyncSendGoal(goal, &goal_id)) {
    AERROR << "[Sync] SyncSendGoal failed at iteration " << index;
    return;
  }

  AINFO << "[Sync] Goal sent: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
  LookUpTransform::Result result;
  GoalStatusCode status = GoalStatusCode::STATUS_UNKNOWN;
  if (!client->WaitForResult(goal_id, &result, &status)) {
    AERROR << "WaitForResult failed at index=" << index;
    return;
  }
  AINFO << "[Sync] Result received: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id)
        << ", status=" << static_cast<int>(status)
        << ", transform=" << result.transform() << ", error=" << result.error();
}

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_client_sync");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto client = node->CreateActionClient<LookUpTransform>("lookup_transform");
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto callback = [&client, &index]() {
    index++;
//Cancel the currently sent goal every 5 times
    if (index > 0 && index % 2 == 0) {
      SyncCancelAfterSendGoal(client, index);
      return;
    }
    SyncSendGoal(client, index);
  };

  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

#### Action Client Common ActionOptions Advanced Properties

(Only the commonly used ones are introduced, sorted by common priority)

- **rpc_timeout_ms**: Action Client waits for Action Server to reply to the Ack timeout of Goal/Cancel/Result request.
- **feedback_mode**: The mode for transmitting feedback to Action Server. Must be consistent with Action Server side
- **wait_server_timeout_ms**: Maximum negotiation time to wait to establish a connection with ActionServer service discovery
- **wait_result_timeout_ms**: After completing the handshake set by the goal with the ActionServer, the timeout time to obtain the final result

```cpp
rti::segar::action::ActionOptions action_options;
action_options.feedback_mode = OptionalMode::RTPS;
action_options.status_mode = OptionalMode::HYBRID;
action_options.get_result_qos = QosProfileConf::QOS_PROFILE_TF_STATIC;
action_options.wait_server_timeout_ms = 5000.0;
action_options.wait_result_timeout_ms = 10000.0;

client = node->CreateActionClient<LookUpTransform>("lookup_transform", callbacks, action_options);
```

### 2.4 Client (Action Client, asynchronous mode)

#### AsyncSendGoal asynchronous function description

- **Parameter 1**: Goal object to be sent
- **Parameter 2**: The GoalID that uniquely marks the Goal obtained after sending
- **Parameter 3**: GoalCallbacks (merged with global callbacks when created, the priority defined in this parameter is higher)
- **Return value**: `true` means the Goal was successfully sent; `false` means the sending failed (unable to establish an effective link based on service discovery with ActionServer)

#### Asynchronous client example

(`src/action_example/action_client_async/src/action_client_async.cc`)

```cpp
#include <chrono>

#include "example/action/LookUpTransform.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_client_async");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using ActionClient =
      rti::segar::action::ActionClient<example::action::LookUpTransform>;
  using GoalID = rti::segar::action::GoalID;
  using GoalStatusCode = rti::segar::action::GoalStatusCode;
  using LookUpTransform = example::action::LookUpTransform;

  ActionClient::GoalCallbacks callbacks;

  callbacks.on_feedback = [](ActionClient& /*client*/, const GoalID& goal_id,
                             const LookUpTransform::Feedback& feedback) {
    AINFO << "[Async] Feedback received: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", current=" << feedback.current();
  };

  callbacks.on_result = [](ActionClient& /*client*/, const GoalID& goal_id,
                           const LookUpTransform::Result& result,
                           GoalStatusCode status) {
    AINFO << "[Async] Result received: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", status=" << static_cast<int>(status)
          << ", transform=" << result.transform() << ", error=" << result.error();
  };

//Register cancel callback
  callbacks.on_cancel = [](ActionClient& /*client*/, const GoalID& goal_id,
                           rti::segar::action::CancelResponseCode code) {
    AINFO << "[Async] Cancel response: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", code=" << static_cast<int>(code);
  };

  auto client =
      node->CreateActionClient<LookUpTransform>("lookup_transform", callbacks);
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto timer_callback = [&client, &index]() {
    ++index;
    LookUpTransform::Goal goal;
    goal.target_frame("map" + std::to_string(index));
    GoalID goal_id;
    if (!client->AsyncSendGoal(goal, &goal_id)) {
      AERROR << "[Async] SendGoal failed at iteration " << index;
      return;
    }
    AINFO << "[Async] Goal sent: iteration=" << index << ", goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id);

//Cancel the currently sent goal every other time
    if (index > 0 && index % 5 == 0) {
// Wait for a while to give feedback a chance to be received
      rti::segar::SleepFor(std::chrono::milliseconds(30));
      AINFO << "[Async] Cancelling goal: goal_id="
            << rti::segar::action::internal::GoalIDToString(goal_id);
      client->AsyncCancelGoal(goal_id);
      return;
    }
  };

  // 1hz
  auto timer =
      std::make_shared<rti::segar::Timer>(1000, timer_callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

---

## 3. CLI Debugging

The `segar action` command can be used to query Action (for detailed functions, please refer to the Segar CLI Getting Started Document):

```bash
$ segar action
info  list type

$ segar action info lookup_transform
Action: lookup_transform
Servers (1):
  node=lookup_transform_server host=simon-virtual-machine pid=59302
Clients (0):
  (none)
```
