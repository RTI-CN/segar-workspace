# Getting started with Segar Python Topic

> Topic message definition follows `.msg` (ROS2 style IDL).

---

## 1. Message definition

Example message:

```text
src/type_src/example/msg/String.msg
```
---

## 2. Python interface main process

Fixed main flow aligned with C++:

1. `segar.Init("module_name")`
2. `node = segar.CreateNode("node_name")`
3. `CreateWriter/CreateReader`
4. `segar.WaitForShutdown()`

---

## 3. Writer example

Corresponding engineering module:

- `src_python/topic_example/topic_talker/src/topic_talker.py`

Key calls:

```python
from example.msg import String
from segar.python import segar

segar.Init("topic_talker")
node = segar.CreateNode("topic_talker")
writer = node.CreateWriter("/topic/chatter", String)
writer.Write(String(data="hello"))
```
---

## 4. Reader example

Corresponding engineering module:

- `src_python/topic_example/topic_listener/src/topic_listener.py`

Key calls:

```python
reader = node.CreateReader("/topic/chatter", String, on_msg)
```
Callback signature:

```python
def on_msg(msg):
    print(msg.data)
```
---

## 5. Optional configuration (opposite to C++ options)

- `CreateWriter(..., options=...)`
- `CreateReader(..., options=...)`

Commonly used keys:

- Writer: `qos_depth`, `qos_profile`, `history`, `depth`, `reliability`, `durability`
- Reader: `pending_queue_size`, `qos_profile`, `history`, `depth`, `reliability`, `durability`

`segar.WriterConfig` / `segar.ReaderConfig` can also be used.

---

## 6. Operation mode

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/topic_example/topic_talker
./scripts/launch.sh
```
