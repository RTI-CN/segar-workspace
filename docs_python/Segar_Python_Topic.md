# Segar Python Topic 使用入门

> Topic 消息定义沿用 `.msg`（ROS2 风格 IDL）。

---

## 1. 消息定义

示例消息：

```text
src/type_src/example/msg/String.msg
```

---

## 2. Python 接口主流程

与 C++ 对齐的固定主流程：

1. `segar.Init("module_name")`
2. `node = segar.CreateNode("node_name")`
3. `CreateWriter/CreateReader`
4. `segar.WaitForShutdown()`

---

## 3. Writer 示例

对应工程模块：

- `src_python/topic_example/topic_talker/src/topic_talker.py`

关键调用：

```python
from example.msg import String
from segar.python import segar

segar.Init("topic_talker")
node = segar.CreateNode("topic_talker")
writer = node.CreateWriter("/topic/chatter", String)
writer.Write(String(data="hello"))
```

---

## 4. Reader 示例

对应工程模块：

- `src_python/topic_example/topic_listener/src/topic_listener.py`

关键调用：

```python
reader = node.CreateReader("/topic/chatter", String, on_msg)
```

回调签名：

```python
def on_msg(msg):
    print(msg.data)
```

---

## 5. 可选配置（与 C++ options 对位）

- `CreateWriter(..., options=...)`
- `CreateReader(..., options=...)`

常用键：

- Writer: `qos_depth`, `qos_profile`, `history`, `depth`, `reliability`, `durability`
- Reader: `pending_queue_size`, `qos_profile`, `history`, `depth`, `reliability`, `durability`

也可使用 `segar.WriterConfig` / `segar.ReaderConfig`。

---

## 6. 运行方式

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/topic_example/topic_talker
./scripts/launch.sh
```
