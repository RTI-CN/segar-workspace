# Segar Action

> **说明**：（可选配置）或（可选阅读）内容都不常用，请酌情了解。
>
> Segar Topic 和 Segar Service 已介绍过的通用基础知识，本文将不再重述。
>
> Action 消息采用 ROS 2 风格的 `.action` 文件定义。
>
> **快速流程**：编写 `.action` → 编写业务代码 → 编译 → 运行 ActionServer/ActionClient 两端示例 → CLI 验证。

---

## 1. .action 文件怎么写

### 1.1 文件位置 / 命名规范 / 类型 / Namespace

文件命名规范遵循 ROS 2 定义。例如定义一个命名空间为 `example` 的 `LookUpTransform` action：

```text
src/type_src/example/action/LookUpTransform.action
```

- 文件名使用 CamelCase（例如 `LookUpTransform.action`）
- `.action` 文件的名字部分即为 Segar Action 类型名
- `.action` 文件的目录即为 namespace，例如 `LookUpTransform` 对应 `namespace example::action;`
- 语法及使用方式同 ROS 2，不再展开介绍

---

## 2. C++ 基本使用示例（非 Component 用法）

示例节选自 `src/action_example/*/action_*.cc`，拆分为 Server 端和 Client 端两个示例。

### 2.1 代码说明

- **（必需）** 包含自动生成的 Action 消息 hpp 文件
- **（必需）** 收发双方的 `action_name` 必须完全一致（例：`lookup_transform`）
- **（必需）** `rti::segar::Init(argv[0]);` 用于初始化 Segar 系统功能
- **（可选）** `rti::segar::WaitForShutdown();` 用于防止系统主线程退出，使用 Ctrl+C 可退出；如果退出前需要执行用户清理逻辑，可以传入 callback

### 2.2 服务端（Action Server，纯异步）

#### ActionServer\<T\>::Callbacks 成员说明

- **on_goal**：处理 ActionClient 的 goal 请求，返回 `false` 表示拒绝该 goal
- **on_cancel**：处理 ActionClient 的 cancel 请求，返回 `false` 表示拒绝该用户 cancel goal
- **on_execute**：处理 ActionClient 的 goal 消息，自主决定如何做出以下响应：
  1. `PublishFeedback(goal_id, feedback)`：发布当前进度
  2. `CancelGoal(goal_id, result)`：中途取消 goal，并将中途 result 发给 Client
  3. `Succeed(goal_id, result)`：成功处理 goal，并将最终 result 发给 Client

#### CreateActionServer 函数说明

- **模板参数**：`.action` 文件定义的 action 类型
- **参数 1**：action name
- **参数 2**：ActionServer 的 Callbacks
- **参数 3**：ActionOptions 高级属性（可选）
- **返回值**：ActionServer 对象

#### 服务端示例

（`src/action_example/action_server/src/action_server.cc`）

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

#### Action Server 常用 ActionOptions 高级属性

（仅介绍常用的，按常用优先级排序）

- **max_execute_concurrency**：Action Server 能并发执行的任务个数
- **max_active_goals**：Action Server 能缓存的未完成任务个数
- **feedback_mode**：向 Action Client 传输 feedback 的模式。要可靠选 `OptionalMode::RTPS`，要效率选 `OptionalMode::HYBRID`。必须和 Action Client 端一致
- **status_publish_period_sec**：向 Action Client 传输 status 的间隔。越小实时性越高，越大资源占用越低
- **status_history_depth**：缓存的历史状态大小，需要全量的就增大数值，只需要当前最新的就设置为 1

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

### 2.3 客户端（Action Client，同步模式）

#### ActionClient\<T\>::GoalCallbacks 成员说明

- **on_accept**：处理从 ActionServer 收到的 Ack Response 回复。`true`：ActionServer 接受该 Goal 请求；`false`：ActionServer 拒绝该 Goal 请求
- **on_result**：该函数仅在异步模式下生效，同步模式（使用 `SyncSendGoal`）下将被忽略。处理从 ActionServer 收到的最终 Result，通过 status 参数获得该 Result 在以下哪种状态下获得的：`STATUS_SUCCEEDED`、`STATUS_CANCELED`、`STATUS_ABORTED`
- **on_cancel**：处理从 ActionServer 收到的 Cancel 处理状态结果（`ERROR_REJECTED`、`ERROR_UNKNOWN_GOAL`、`ERROR_TIMEOUT`、`ERROR_NOT_CANCELABLE`、`SUCCESS_REQUESTED`）
- **on_feedback**：处理从 ActionServer 收到的 Feedback 消息

#### CreateActionClient 函数说明

- **模板参数**：`.action` 文件定义的 action 类型
- **参数 1**：action name
- **参数 2**：GoalCallbacks，后续的 `SendGoal`、`SyncSendGoal`、`CancelGoal` 都使用该参数定义的 callbacks，可在此基础上额外覆盖
- **参数 3**：ActionOptions 高级属性（可选）
- **返回值**：ActionClient 对象

#### SyncSendGoal 同步函数说明

- **参数 1**：要发送的 Goal 对象
- **参数 2**：发送后得到的唯一标记该 Goal 的 GoalID
- **参数 3**：GoalCallbacks（与创建时的全局 callbacks 合并，该参数内定义的优先级更高）
- **返回值**：`true` 表示成功发送 Goal 且收到 response 且 ActionServer 接受了该 Goal；`false` 表示 otherwise

#### WaitForResult 同步函数说明

- **参数 1**：`SyncSendGoal` 生成的 GoalID
- **参数 2**：存储最终 Result 的对象引用
- **参数 3**：存储 Result 状态的对象引用，可得到 `STATUS_SUCCEEDED`、`STATUS_CANCELED`、`STATUS_ABORTED`
- **返回值**：`true` 表示按时获取到最终 Result；`false` 表示获取 Result 超时

#### 同步客户端示例

（`src/action_example/action_client_sync/src/action_client_sync.cc`）

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
  // 等待 goal 真正开始执行
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
    // 每隔 5 次取消当前发送的 goal
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

#### Action Client 常用 ActionOptions 高级属性

（仅介绍常用的，按常用优先级排序）

- **rpc_timeout_ms**：Action Client 等待 Action Server 回复 Goal/Cancel/Result request 的 Ack 超时时间
- **feedback_mode**：向 Action Server 传输 feedback 的模式。必须和 Action Server 端一致
- **wait_server_timeout_ms**：等待与 ActionServer 服务发现建立连接的最大协商时间
- **wait_result_timeout_ms**：在与 ActionServer 完成 goal 设定的握手后，获取最终 result 的超时时间

```cpp
rti::segar::action::ActionOptions action_options;
action_options.feedback_mode = OptionalMode::RTPS;
action_options.status_mode = OptionalMode::HYBRID;
action_options.get_result_qos = QosProfileConf::QOS_PROFILE_TF_STATIC;
action_options.wait_server_timeout_ms = 5000.0;
action_options.wait_result_timeout_ms = 10000.0;

client = node->CreateActionClient<LookUpTransform>("lookup_transform", callbacks, action_options);
```

### 2.4 客户端（Action Client，异步模式）

#### AsyncSendGoal 异步函数说明

- **参数 1**：要发送的 Goal 对象
- **参数 2**：发送后得到的唯一标记该 Goal 的 GoalID
- **参数 3**：GoalCallbacks（与创建时的全局 callbacks 合并，该参数内定义的优先级更高）
- **返回值**：`true` 表示成功发送 Goal；`false` 表示发送失败（无法和 ActionServer 建立基于服务发现的有效链路）

#### 异步客户端示例

（`src/action_example/action_client_async/src/action_client_async.cc`）

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

  // 注册 cancel 回调
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

    // 每隔一次取消当前发送的 goal
    if (index > 0 && index % 5 == 0) {
      // 等待一段时间让 feedback 有机会被接收
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

## 3. CLI 调试

`segar action` 命令可用于查询 Action（详细功能请参见 Segar CLI 文档）：

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
