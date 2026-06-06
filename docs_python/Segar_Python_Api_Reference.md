# Segar Python API 参考手册

> 本文列出 Python 侧最常用通信 API，签名以 `segar/python/segar.py` 与 `segar/python/parameter.py` 当前实现为准。

---

## 1. 初始化与节点

| API | 签名 | 说明 |
|---|---|---|
| Init | `segar.Init(module_name="segar_py") -> bool` | 初始化运行时 |
| OK | `segar.OK() -> bool` | 运行状态是否正常 |
| Shutdown | `segar.Shutdown() -> bool` | 主动触发退出 |
| WaitForShutdown | `segar.WaitForShutdown() -> bool` | 阻塞等待退出 |
| CreateNode | `segar.CreateNode(node_name, name_space="") -> Node` | 创建节点 |

---

## 2. Topic

| API | 签名 | 说明 |
|---|---|---|
| CreateWriter | `node.CreateWriter(name, data_type, qos_depth=1, options=None)` | 创建 Writer |
| CreateReader | `node.CreateReader(name, data_type, callback, args=None, options=None)` | 创建 Reader |
| Write | `writer.Write(msg) -> bool/int` | 发送消息 |

`data_type` 支持：

- protobuf 消息类型
- 生成的 IDL 消息类型（`.msg`）
- `"RawData"`（字节流）

---

## 3. Service

| API | 签名 | 说明 |
|---|---|---|
| CreateService（IDL） | `node.CreateService(name, service_type, callback[, args])` | 创建 IDL Service |
| CreateService（Proto） | `node.CreateService(name, req_type, res_type, callback[, args])` | 创建 Proto Service |
| CreateClient（IDL） | `node.CreateClient(name, service_type[, options])` | 创建 IDL Client |
| CreateClient（Proto） | `node.CreateClient(name, req_type, res_type[, options])` | 创建 Proto Client |
| SyncSendRequest | `client.SyncSendRequest(data, timeout_sec=None, options=None)` | 同步请求 |
| AsyncSendRequest | `client.AsyncSendRequest(data, callback=None, args=None, options=None)` | 异步请求 |

---

## 4. Action

| API | 签名 | 说明 |
|---|---|---|
| CreateActionServer | `node.CreateActionServer(name, action_type, callbacks, options=None)` | 创建 Action Server |
| CreateActionClient | `node.CreateActionClient(name, action_type, options=None, callbacks=None)` | 创建 Action Client |
| SyncSendGoal | `client.SyncSendGoal(goal, out_goal_id) -> bool` | 同步发送 goal |
| AsyncSendGoal | `client.AsyncSendGoal(goal, out_goal_id, callbacks=None) -> bool` | 异步发送 goal |
| WaitForResult | `client.WaitForResult(goal_id) -> ActionResult/None` | 等待结果 |
| SyncCancelGoal | `client.SyncCancelGoal(goal_id, callbacks=None, timeout_sec=2.0) -> bool` | 同步取消 |
| AsyncCancelGoal | `client.AsyncCancelGoal(goal_id, callbacks=None, timeout_sec=0.0) -> bool` | 异步取消 |
| WaitForServer | `client.WaitForServer(timeout_sec=2.0) -> bool` | 等待服务端可用 |

Server 执行期 API：

- `server.PublishFeedback(goal_id, feedback)`
- `server.Succeed(goal_id, result)`
- `server.Abort(goal_id, result)`
- `server.CancelGoal(goal_id, result)`
- `server.IsCancelRequested(goal_id)`

---

## 5. Parameter

导入：

```python
from segar.python import parameter
```

| API | 签名 | 说明 |
|---|---|---|
| Segar_Set_Local_Param | `(node, param_name, value) -> bool` | 设置本地参数 |
| Segar_Get_Local_Param | `(node, param_name, out_value, value_type=None) -> bool` | 获取本地参数 |
| Segar_List_Local_Params | `(node, out_list) -> bool` | 列出本地参数 |
| Segar_Load_Local_Params | `(node, yaml_file) -> bool` | 从 YAML 加载本地参数 |
| Segar_Dump_Local_Params | `(node, yaml_file) -> bool` | 导出本地参数 |
| Segar_Set_Remote_Param | `(service_node_name, param_name, value) -> bool` | 设置远程参数 |
| Segar_Get_Remote_Param | `(service_node_name, param_name, out_value, value_type=None) -> bool` | 获取远程参数 |
| Segar_List_Remote_Params | `(service_node_name, out_list) -> bool` | 列出远程参数 |
| Segar_Load_Remote_Params | `(service_node_name, yaml_file) -> bool` | 加载远程参数 |
| Segar_Dump_Remote_Params | `(service_node_name, yaml_file) -> bool` | 导出远程参数 |

---

## 6. Time / Duration

导入：

```python
from segar.python import segar_time
```

### 6.1 `Duration`

| API | 签名 | 说明 |
|---|---|---|
| Duration | `segar_time.Duration(other)` | 创建时长对象 |
| sleep | `duration.sleep() -> None` | 按当前 Duration 睡眠 |
| to_sec | `duration.to_sec() -> float` | 转秒 |
| to_nsec | `duration.to_nsec() -> int` | 转纳秒 |
| iszero | `duration.iszero() -> bool` | 是否为 0 |

构造规则：

- `Duration(int)`：按**纳秒**
- `Duration(float)`：按**秒**
- `Duration(Duration)`：拷贝已有对象

例如：

```python
segar_time.Duration(50000000)   # 50000000 ns
segar_time.Duration(0.05)       # 0.05 s = 50 ms
```

### 6.2 `Time`

| API | 签名 | 说明 |
|---|---|---|
| Time | `segar_time.Time(other)` | 创建时间对象 |
| now | `segar_time.Time.now() -> Time` | 当前时间 |
| mono_time | `segar_time.Time.mono_time() -> Time` | 单调时钟 |
| to_sec | `time.to_sec() -> float` | 转秒 |
| to_nsec | `time.to_nsec() -> int` | 转纳秒 |
| sleep_until | `time.sleep_until(target_time) -> None` | 睡到目标时刻 |

构造规则：

- `Time(int)`：按**纳秒**
- `Time(float)`：按**秒**
- `Time(Time)`：拷贝已有对象

组合规则：

- `Time + Duration -> Time`
- `Time - Duration -> Time`
- `Time - Time -> Duration`

例如：

```python
deadline = segar_time.Time.now() + segar_time.Duration(0.05)
```

---

## 7. 常用辅助类型

- `segar.WriterConfig`
- `segar.ReaderConfig`
- `segar.RequestOptions`
- `segar.ActionOptions`
- `segar.ActionClientCallbacks`
- `segar.ActionServerCallbacks`
- `parameter.OutValue`
