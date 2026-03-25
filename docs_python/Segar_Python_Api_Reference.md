# Segar Python API Reference Manual

> This article lists the most commonly used communication APIs on the Python side. The signatures are subject to the current implementation of `segar/python/segar.py` and `segar/python/parameter.py`.

---

## 1. Initialization and nodes

| API | Signature | Description |
|---|---|---|
| Init | `segar.Init(module_name="segar_py") -> bool` | Initialize runtime |
| OK | `segar.OK() -> bool` | Is the running status normal |
| Shutdown | `segar.Shutdown() -> bool` | Actively trigger exit |
| WaitForShutdown | `segar.WaitForShutdown() -> bool` | Block waiting to exit |
| CreateNode | `segar.CreateNode(node_name, name_space="") -> Node` | Create node |

---

## 2. Topic

| API | Signature | Description |
|---|---|---|
| CreateWriter | `node.CreateWriter(name, data_type, qos_depth=1, options=None)` | Create Writer |
| CreateReader | `node.CreateReader(name, data_type, callback, args=None, options=None)` | Create Reader |
| Write | `writer.Write(msg) -> bool/int` | Send message |

`data_type` supports:

- protobuf message type
- Generated IDL message type (`.msg`)
- `"RawData"` (byte stream)

---

## 3. Service

| API | Signature | Description |
|---|---|---|
| CreateService (IDL) | `node.CreateService(name, service_type, callback[, args])` | Create IDL Service |
| CreateService (Proto) | `node.CreateService(name, req_type, res_type, callback[, args])` | Create Proto Service |
| CreateClient(IDL) | `node.CreateClient(name, service_type[, options])` | Create IDL Client |
| CreateClient(Proto) | `node.CreateClient(name, req_type, res_type[, options])` | Create Proto Client |
| SyncSendRequest | `client.SyncSendRequest(data, timeout_sec=None, options=None)` | Synchronization request |
| AsyncSendRequest | `client.AsyncSendRequest(data, callback=None, args=None, options=None)` | Asynchronous request |

---

## 4. Action

| API | Signature | Description |
|---|---|---|
| CreateActionServer | `node.CreateActionServer(name, action_type, callbacks, options=None)` | Create Action Server |
| CreateActionClient | `node.CreateActionClient(name, action_type, options=None, callbacks=None)` | Create Action Client |
| SyncSendGoal | `client.SyncSendGoal(goal, out_goal_id) -> bool` | Send goal synchronously |
| AsyncSendGoal | `client.AsyncSendGoal(goal, out_goal_id, callbacks=None) -> bool` | Asynchronously send goal |
| WaitForResult | `client.WaitForResult(goal_id) -> ActionResult/None` | Wait for the result |
| SyncCancelGoal | `client.SyncCancelGoal(goal_id, callbacks=None, timeout_sec=2.0) -> bool` | Sync cancellation |
| AsyncCancelGoal | `client.AsyncCancelGoal(goal_id, callbacks=None, timeout_sec=0.0) -> bool` | Asynchronous cancellation |
| WaitForServer | `client.WaitForServer(timeout_sec=2.0) -> bool` | Wait for the server to be available |

Server execution API:

- `server.PublishFeedback(goal_id, feedback)`
- `server.Succeed(goal_id, result)`
- `server.Abort(goal_id, result)`
- `server.CancelGoal(goal_id, result)`
- `server.IsCancelRequested(goal_id)`

---

## 5. Parameter

Import:

```python
from segar.python import parameter
```
| API | Signature | Description |
|---|---|---|
| Segar_Set_Local_Param | `(node, param_name, value) -> bool` | Set local parameters |
| Segar_Get_Local_Param | `(node, param_name, out_value, value_type=None) -> bool` | Get local parameters |
| Segar_List_Local_Params | `(node, out_list) -> bool` | List local parameters |
| Segar_Load_Local_Params | `(node, yaml_file) -> bool` | Load local parameters from YAML |
| Segar_Dump_Local_Params | `(node, yaml_file) -> bool` | Export local parameters |
| Segar_Set_Remote_Param | `(service_node_name, param_name, value) -> bool` | Set remote parameters |
| Segar_Get_Remote_Param | `(service_node_name, param_name, out_value, value_type=None) -> bool` | Get remote parameters |
| Segar_List_Remote_Params | `(service_node_name, out_list) -> bool` | List remote parameters |
| Segar_Load_Remote_Params | `(service_node_name, yaml_file) -> bool` | Load remote parameters |
| Segar_Dump_Remote_Params | `(service_node_name, yaml_file) -> bool` | Export remote parameters |

---

## 6. Common auxiliary types

- `segar.WriterConfig`
- `segar.ReaderConfig`
- `segar.RequestOptions`
- `segar.ActionOptions`
- `segar.ActionClientCallbacks`
- `segar.ActionServerCallbacks`
- `parameter.OutValue`
