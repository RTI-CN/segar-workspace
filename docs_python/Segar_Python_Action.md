# Segar Python Action 使用入门

> Action 消息定义沿用 `.action`（ROS2 风格 IDL）。

---

## 1. 消息定义

示例 Action：

```text
src/type_src/example/action/LookUpTransform.action
```

---

## 2. Action Server

对应工程模块：

- `src_python/action_example/action_server/src/action_server.py`

关键调用：

```python
callbacks = segar.ActionServerCallbacks(
    on_goal=on_goal,
    on_cancel=on_cancel,
    on_execute=on_execute,
)
server = node.CreateActionServer("lookup_transform", LookUpTransform, callbacks=callbacks)
```

`on_execute` 内常用：

- `server.PublishFeedback(goal_id, feedback)`
- `server.IsCancelRequested(goal_id)`
- `server.CancelGoal(goal_id, result)`
- `server.Succeed(goal_id, result)`

---

## 3. Action Client（同步）

对应工程模块：

- `src_python/action_example/action_client_sync/src/action_client_sync.py`

关键调用：

```python
client = node.CreateActionClient("lookup_transform", LookUpTransform)
client.WaitForServer(3.0)

goal_id = []
client.SyncSendGoal(goal, goal_id)   # bool + out_goal_id(list)
result = client.WaitForResult(goal_id[0])
client.SyncCancelGoal(goal_id[0])
```

`WaitForServer()` 返回 `False` 表示等待超时。

`SyncSendGoal()`、`SyncCancelGoal()` 返回 `False` 表示请求没有成功发出或超时。

`WaitForResult()` 返回 `None` 表示等待结果失败或超时。

---

## 4. Action Client（异步）

对应工程模块：

- `src_python/action_example/action_client_async/src/action_client_async.py`

关键调用：

```python
callbacks = segar.ActionClientCallbacks(
    on_feedback=on_feedback,
    on_result=on_result,
    on_cancel=on_cancel,
)
client = node.CreateActionClient("lookup_transform", LookUpTransform, callbacks=callbacks)

goal_id = []
client.AsyncSendGoal(goal, goal_id)
client.AsyncCancelGoal(goal_id[0])
```

`AsyncSendGoal()`、`AsyncCancelGoal()` 返回 `False` 表示请求没有成功发出。

异步 `on_result`、`on_feedback`、`on_cancel` 回调只在收到对应事件后触发；请求失败或超时时不保证触发回调。

---

## 5. ActionOptions（可选）

可通过 `CreateActionClient/CreateActionServer(..., options=...)` 传入：

- `service_mode`, `feedback_mode`, `status_mode`
- `max_server_active_goals`, `max_server_concurrent_goals`
- `max_client_active_goals`, `max_client_concurrent_goals`
- `rpc_timeout_ms`, `wait_result_timeout_ms`, `wait_server_timeout_ms`
- `send_goal_qos`, `get_result_qos`, `cancel_goal_qos`, `feedback_qos`, `status_qos`

支持字典或 `segar.ActionOptions(...).ToDict()`。

---

## 6. 运行方式

直接使用安装目录下示例自带的 `scripts/launch.sh` 运行即可，脚本会加载 `segar_setup.bash`，设置 `PYTHONPATH`、`LD_LIBRARY_PATH` 等运行环境。

如果手动运行 Python 脚本，需要确保：

- Python 能 import 到 `segar` 和消息包，例如 `example.action`。
- 运行时能找到消息库 `.so`。

```bash
cd build_x86/output/src_python/action_example/action_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/action_example/action_client_async
./scripts/launch.sh
```

或：

```bash
cd build_x86/output/src_python/action_example/action_client_sync
./scripts/launch.sh
```
