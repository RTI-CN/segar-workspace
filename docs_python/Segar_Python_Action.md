# Getting started with Segar Python Action

> Action message definition follows `.action` (ROS2 style IDL).

---

## 1. Message definition

Example Action:

```text
src/type_src/example/action/LookUpTransform.action
```
---

## 2. Action Server

Corresponding engineering module:

- `src_python/action_example/action_server/src/action_server.py`

Key calls:

```python
callbacks = segar.ActionServerCallbacks(
    on_goal=on_goal,
    on_cancel=on_cancel,
    on_execute=on_execute,
)
server = node.CreateActionServer("lookup_transform", LookUpTransform, callbacks=callbacks)
```
Commonly used in `on_execute`:

- `server.PublishFeedback(goal_id, feedback)`
- `server.IsCancelRequested(goal_id)`
- `server.CancelGoal(goal_id, result)`
- `server.Succeed(goal_id, result)`

---

## 3. Action Client (synchronization)

Corresponding engineering module:

- `src_python/action_example/action_client_sync/src/action_client_sync.py`

Key calls:

```python
client = node.CreateActionClient("lookup_transform", LookUpTransform)
client.WaitForServer(3.0)

goal_id = []
client.SyncSendGoal(goal, goal_id)   # bool + out_goal_id(list)
result = client.WaitForResult(goal_id[0])
client.SyncCancelGoal(goal_id[0])
```
---

## 4. Action Client (asynchronous)

Corresponding engineering module:

- `src_python/action_example/action_client_async/src/action_client_async.py`

Key calls:

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
---

## 5. ActionOptions (optional)

Can be passed in through `CreateActionClient/CreateActionServer(..., options=...)`:

- `service_mode`, `feedback_mode`, `status_mode`
- `max_server_active_goals`, `max_server_concurrent_goals`
- `max_client_active_goals`, `max_client_concurrent_goals`
- `rpc_timeout_ms`, `wait_result_timeout_ms`, `wait_server_timeout_ms`
- `send_goal_qos`, `get_result_qos`, `cancel_goal_qos`, `feedback_qos`, `status_qos`

Supports dictionaries or `segar.ActionOptions(...).ToDict()`.

---

## 6. Operation mode

```bash
cd build_x86/output/src_python/action_example/action_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/action_example/action_client_async
./scripts/launch.sh
```
or:

```bash
cd build_x86/output/src_python/action_example/action_client_sync
./scripts/launch.sh
```
