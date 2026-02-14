# Getting Started with Service

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> The general basic knowledge has been introduced in the introduction to using Segar Topic and will not be repeated in this article.
>
> Service messages are defined in ROS 2-style `.srv` files.
>
> **Quick process**: Write `.srv` → Write business code → Compile → Run Server/Client examples on both ends → CLI verification.

---

## 1. How to write .srv file

### 1.1 File Location/Naming Convention/Type/Namespace

File naming convention follows ROS 2 definition. For example, define a `SetCameraInfo` service message with namespace `example`:

```text
src/type_src/example/srv/SetCameraInfo.srv
```

- Use CamelCase for filenames (e.g. `SetCameraInfo.srv`)
- The name part of the `.srv` file is the Segar service type name
- The directory of the `.srv` file is the namespace, for example, `SetCameraInfo` corresponds to `namespace example::srv;`
- The syntax and subsequent usage are the same as ROS 2 and will not be introduced further.

---

## 2. Basic usage examples of C++ (without Components)

The example is excerpted from `src/service_example/`, split into two examples: Server side and Client side.

### 2.1 Code description

- **(required)** Contains automatically generated srv message hpp file
- **(required)** `service_name` of both sender and receiver must be exactly the same (example: `set_camera_info`)
- **(required)** `rti::segar::Init(argv[0]);` is used to initialize Segar system functions
- **(optional)** `rti::segar::WaitForShutdown();` is used to prevent the system main thread from exiting. Use Ctrl+C to exit.

### 2.2 Server (Server, fully asynchronous)

#### CreateService function description

- **Template parameters**: service type defined in `.srv` file
- **Parameter 1**: service name
- **Parameter 2**: Process request and generate response callback
- **Return value**: Service object

#### Server example

(`src/service_example/service_server/src/service_server.cc`)

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

### 2.3 Client (Client, synchronous mode)

#### CreateClient function description

- **Template parameters**: service type defined in `.srv` file
- **Parameter 1**: service name
- **Return value**: Client object

#### Sync client example

(`src/service_example/service_client_sync/src/service_client_sync.cc`)

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

### 2.4 Client (Client, asynchronous mode)

#### SendRequest asynchronous function description

- **Parameter 1**: request created by user
- **Parameter 2**: The callback responsible for processing the response returned by the service
- **Return value**: Client object

#### Asynchronous client example

(`src/service_example/service_client_async/src/service_client_async.cc`)

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

### 2.5 (Optional Reading) RequestOptions Advanced Properties Example

Do not set these unless needed.

- Timeout for waiting for the final response: `timeout` (default 5000ms, modify as needed)
- Time to wait for Service discovery and matching: `wait_service_timeout_ms` (default 2000ms, usually use the default value)
- Whether to wait for Service availability: `ensure_service_available` (do not modify this value)

```cpp
ClientType::RequestOptions options;
options.timeout = std::chrono::milliseconds(1000);
options.wait_service_timeout_ms = std::chrono::milliseconds(3000);

client->SyncSendRequest(request, options);
// or
client->SendRequest(
    request,
    [](const std::shared_ptr<SetCameraInfo::Response>& response) {
       ...
    },
    options);
```

---

## 3. CLI Debugging

The `segar service` command can be used to query the Service (for detailed functions, please refer to the Segar CLI Getting Started Document):

```bash
$ segar service
usage: segar service [-h] {list,type,info} ...

Various service related utilities

commands:
  list                        List all services
  type <ServiceName>          Display .srv type for a service
  info <ServiceName>          Display info of service
```
