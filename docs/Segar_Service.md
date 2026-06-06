# Segar Service

> **说明**：（可选配置）或（可选阅读）内容都不常用，请酌情了解。
>
> Segar Topic 已介绍过的通用基础知识，本文将不再重述。
>
> Service 消息采用 ROS 2 风格的 `.srv` 文件定义。
>
> **快速流程**：编写 `.srv` → 编写业务代码 → 编译 → 运行 Server/Client 两端示例 → CLI 验证。

---

## 1. .srv 文件怎么写

### 1.1 文件位置 / 命名规范 / 类型 / Namespace

文件命名规范遵循 ROS 2 定义。例如定义一个命名空间为 `example` 的 `SetCameraInfo` 服务消息：

```text
src/type_src/example/srv/SetCameraInfo.srv
```

- 文件名使用 CamelCase（例如 `SetCameraInfo.srv`）
- `.srv` 文件的名字部分即为 Segar 服务类型名
- `.srv` 文件的目录即为 namespace，例如 `SetCameraInfo` 对应 `namespace example::srv;`
- 语法及后续使用方式同 ROS 2，不再展开介绍

---

## 2. C++ 基本使用示例（非 Component 用法）

示例节选自 `src/service_example/`，拆分为 Server 端和 Client 端两个示例。

### 2.1 代码说明

- **（必需）** 包含自动生成的 srv 消息 hpp 文件
- **（必需）** 收发双方的 `service_name` 必须完全一致（例：`set_camera_info`）
- **（必需）** `rti::segar::Init(argv[0]);` 用于初始化 Segar 系统功能
- **（可选）** `rti::segar::WaitForShutdown();` 用于防止系统主线程退出，使用 Ctrl+C 可退出；如果退出前需要执行用户清理逻辑，可以传入 callback

### 2.2 服务端（Server，纯异步）

#### CreateService 函数说明

- **模板参数**：`.srv` 文件定义的 service 类型
- **参数 1**：service name
- **参数 2**：处理 request 并生成 response 的 callback
- **返回值**：Service 对象

#### 服务端示例

（`src/service_example/service_server/src/service_server.cc`）

```cpp
#include <memory>
#include <string>

#include "example/srv/SetCameraInfo.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("set_camera_info_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using SetCameraInfo = example::srv::SetCameraInfo;
  auto callback = [](const std::shared_ptr<SetCameraInfo::Request>& request,
                     std::shared_ptr<SetCameraInfo::Response>& response) {
    response->success(true);
    response->status_message("Camera info set successfully");
    AINFO << "Request camera width: " << request->camera_info().width()
          << ", Response msg:" << response->status_message();
  };
  auto service =
      node->CreateService<SetCameraInfo>("set_camera_info", callback);
  RETURN_VAL_IF(!service, EXIT_FAILURE);

  AINFO << "Waiting for requests...";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

### 2.3 客户端（Client，同步模式）

#### CreateClient 函数说明

- **模板参数**：`.srv` 文件定义的 service 类型
- **参数 1**：service name
- **返回值**：Client 对象

#### 同步客户端示例

（`src/service_example/service_client_sync/src/service_client_sync.cc`）

```cpp
#include <memory>

#include "example/srv/SetCameraInfo.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("set_camera_info_client_sync");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using SetCameraInfo = example::srv::SetCameraInfo;
  auto client = node->CreateClient<SetCameraInfo>("set_camera_info");
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto callback = [&client, &index]() {
    auto request = std::make_shared<SetCameraInfo::Request>();
    request->camera_info().width(index);

    auto response = client->SyncSendRequest(request);
    if (response != nullptr) {
      AINFO << "[Sync] request camera width: " << index
            << ", Response msg:" << response->status_message();
    } else {
      AINFO << "[Sync] Request failed, request camera width: " << index;
    }

    index++;
  };

  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

### 2.4 客户端（Client，异步模式）

#### SendRequest 异步函数说明

- **参数 1**：用户创建的 request
- **参数 2**：负责处理 service 返回的 response 的 callback
- **返回值**：是否成功发起请求；返回 `false` 时通常表示 Client 未就绪或请求被拒绝

#### 异步客户端示例

（`src/service_example/service_client_async/src/service_client_async.cc`）

```cpp
#include <memory>

#include "example/srv/SetCameraInfo.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("set_camera_info_client_async");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using SetCameraInfo = example::srv::SetCameraInfo;
  auto client = node->CreateClient<SetCameraInfo>("set_camera_info");
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto timer_callback = [&client, &index]() {
    auto request = std::make_shared<SetCameraInfo::Request>();
    request->camera_info().width(index);

    auto response_callback =
        [index](const std::shared_ptr<SetCameraInfo::Response>& response) {
          if (response) {
            AINFO << "[Async] request camera width: " << index
                  << ", Response msg:" << response->status_message();
          } else {
            AINFO << "[Async] Request failed, request camera width: " << index;
          }
        };
    client->AsyncSendRequest(request, response_callback);
    AINFO << "[Async] request camera width: " << index;
    index++;
  };

  // 1hz
  auto timer =
      std::make_shared<rti::segar::Timer>(1000, timer_callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
```

### 2.5 （可选阅读）RequestOptions 高级属性示例

非必要别设置。

- 等待最终 Response 的超时时间：`timeout`（默认 5000ms，根据需要修改）
- 等待与 Service 完成配对的时间：`wait_service_timeout_ms`（默认 2000ms，通常使用默认值）
- 是否开启对 Service 完成配对的等待：`ensure_service_available`（不要修改该值）

```cpp
ClientType::RequestOptions options;
options.timeout = std::chrono::milliseconds(1000);
options.wait_service_timeout_ms = std::chrono::milliseconds(3000);

client->SyncSendRequest(request, options);
// 或
client->SendRequest(
    request,
    [](const std::shared_ptr<SetCameraInfo::Response>& response) {
       ...
    },
    options);
```

---

## 3. CLI 调试

`segar service` 命令可用于查询 Service（详细功能请参见 Segar CLI 文档）：

```bash
$ segar service
usage: segar service [-h] {list,type,info} ...

Various service related utilities

commands:
  list                        List all services
  type <ServiceName>          Display .srv type for a service
  info <ServiceName>          Display info of service
```
