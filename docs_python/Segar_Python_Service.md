# Getting started with Segar Python Service

> Service message definition follows `.srv` (ROS2 style IDL).

---

## 1. Message definition

Example service:

```text
src/type_src/example/srv/SetCameraInfo.srv
```
---

## 2. Service Server

Corresponding engineering module:

- `src_python/service_example/service_server/src/service_server.py`

Key calls:

```python
from example.srv import SetCameraInfo
from segar.python import segar

def on_request(request):
    return SetCameraInfo.Response(success=True, status_message="ok")

service = node.CreateService("set_camera_info", SetCameraInfo, on_request)
```
---

## 3. Service Client (synchronization)

Corresponding engineering module:

- `src_python/service_example/service_client_sync/src/service_client_sync.py`

Key calls:

```python
client = node.CreateClient("set_camera_info", SetCameraInfo)
response = client.SyncSendRequest(request)
```
`response is None` means the request failed or timed out.

---

## 4. Service Client (asynchronous)

Corresponding engineering module:

- `src_python/service_example/service_client_async/src/service_client_async.py`

Key calls:

```python
def on_response(response, req_idx):
    ...

ok = client.AsyncSendRequest(request, callback=on_response, args=index)
```
---

## 5. RequestOptions

Optional request configuration:

- `timeout_ms`
- `wait_service_timeout_ms`
- `ensure_service_available`
- `is_sync`

Two methods of transmission:

- Dictionary `options={...}`
- `segar.RequestOptions(...).ToDict()`

---

## 6. Operation mode

```bash
cd build_x86/output/src_python/service_example/service_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/service_example/service_client_sync
./scripts/launch.sh
```
or:

```bash
cd build_x86/output/src_python/service_example/service_client_async
./scripts/launch.sh
```
