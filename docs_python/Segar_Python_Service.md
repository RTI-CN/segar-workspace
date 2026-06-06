# Segar Python Service 使用入门

> Service 消息定义沿用 `.srv`（ROS2 风格 IDL）。

---

## 1. 消息定义

示例服务：

```text
src/type_src/example/srv/SetCameraInfo.srv
```

---

## 2. Service Server

对应工程模块：

- `src_python/service_example/service_server/src/service_server.py`

关键调用：

```python
from example.srv import SetCameraInfo
from segar.python import segar

def on_request(request):
    return SetCameraInfo.Response(success=True, status_message="ok")

service = node.CreateService("set_camera_info", SetCameraInfo, on_request)
```

---

## 3. Service Client（同步）

对应工程模块：

- `src_python/service_example/service_client_sync/src/service_client_sync.py`

关键调用：

```python
client = node.CreateClient("set_camera_info", SetCameraInfo)
response = client.SyncSendRequest(request)
```

`response is None` 表示请求失败或超时。

---

## 4. Service Client（异步）

对应工程模块：

- `src_python/service_example/service_client_async/src/service_client_async.py`

关键调用：

```python
def on_response(response, req_idx):
    ...

ok = client.AsyncSendRequest(request, callback=on_response, args=index)
```

`AsyncSendRequest()` 返回 `False` 表示请求没有成功发出。

异步回调中的 `response is None` 表示请求失败或超时。

---

## 5. RequestOptions

可选请求配置：

- `timeout_ms`
- `wait_service_timeout_ms`
- `ensure_service_available`
- `is_sync`

两种传法：

- 字典 `options={...}`
- `segar.RequestOptions(...).ToDict()`

示例：

```python
client = node.CreateClient(
    "set_camera_info",
    SetCameraInfo,
    {"timeout_ms": 200},
)
```

---

## 6. 运行前提

直接使用安装目录下示例自带的 `scripts/launch.sh` 运行即可，脚本会加载 `segar_setup.bash`，设置 `PYTHONPATH`、`LD_LIBRARY_PATH` 等运行环境。

如果手动运行 Python 脚本，需要确保：

- Python 能 import 到 `segar` 和消息包，例如 `example.srv`。
- 运行时能找到消息库 `.so`。

---

## 7. 运行方式

```bash
cd build_x86/output/src_python/service_example/service_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/service_example/service_client_sync
./scripts/launch.sh
```

或：

```bash
cd build_x86/output/src_python/service_example/service_client_async
./scripts/launch.sh
```
