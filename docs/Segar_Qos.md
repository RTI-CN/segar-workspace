# Segar QoS

> **说明**：QoS 用于描述 Topic/Service/Action 等通信实体的“传输质量”与“历史/补发行为”。**本文档仅针对 Topic 的 QoS 进行说明**，Service/Action 的 QoS 不在本文档范围内；如有需求请联系开发团队咨询。
>
> **快速流程**：优先在 `.dag` 中配置 `qos_profile` →在代码中覆盖 `qos_profile` →针对 late-join/静态数据使用 transient_local。

---

## 0. 重要说明：`qos_profile` 的生效场景

- **RTPS（跨主机/显式 RTPS）**：`qos_profile` **生效**
  - 典型生效项：`history/depth/reliability/durability/mps`
- **SHM（跨进程同机）/INTRA（同进程）**：除 `durability=TRANSIENT_LOCAL` 相关机制外，其它 `qos_profile` 配置 **不生效**
- **仅在 `durability=TRANSIENT_LOCAL` 时才会启用的能力**：历史缓存与 late-join 补发（主要使用 `history/depth`）

---

## 1. 推荐用法：在 `.dag` 中为 Reader 配置 `qos_profile`（优先推荐）

Segar 的组件化（Component）场景中，Reader 往往通过 `.dag` 文件统一配置。**推荐优先使用这种方式**，原因：

- **集中管理**：不用在代码里散落多个 topic 的 QoS hardcode
- **可配置**：同一组件在不同场景可用不同 `.dag` 配置复用
- **更贴近运行期**：`pending_queue_size`/topic 列表/QoS 放在一个地方便于联调

### 1.1 示例文件路径

```text
src/component_example/common_component/config/common.dag
```

### 1.2 示例说明

```text
# CommonComponentExample subscribes: /topic/chatter, /topic/image_front, /topic/image_rear
module_config {
    module_library : "lib/libcommon_component.so"
    components {
        component_class_name : "CommonComponentExample"
        config {
            inner_node_name: "common_component_example"
            readers {
                topic: "/topic/chatter"
                pending_queue_size: 5
                qos_profile {
                    depth: 10
                    history: HISTORY_KEEP_LAST
                }
            }
            readers {
                topic: "/topic/image_front"
                pending_queue_size: 5
                qos_profile {
                    depth: 10
                    history: HISTORY_KEEP_LAST
                }
            }
            readers {
                topic: "/topic/image_rear"
                pending_queue_size: 5
                qos_profile {
                    depth: 10
                    history: HISTORY_KEEP_LAST
                }
            }
        }
      }
    }
}
```

`qos_profile` 还可按需显式写出更多项（含义见 §1.3）。每个 `readers` 里把上面内层替换成如下即可：

```text
                qos_profile {
                    depth: 10
                    history: HISTORY_KEEP_LAST
                    reliability: RELIABILITY_RELIABLE
                    durability: DURABILITY_VOLATILE
                    mps: 0
                }
```

### 1.3 字段解释

- **topic**：订阅的 topic 名称
- **pending_queue_size**：Reader 回调侧“未处理消息”的缓存队列长度（用于应对 callback 忙导致的堆积）
- **qos_profile.depth**：history 容量（通常表示保留最近 N 条）
- **qos_profile.history**：历史策略
  - `HISTORY_KEEP_LAST`：保留最近 `depth` 条
  - `HISTORY_KEEP_ALL`：尽可能保留全部（仍受资源限制影响），尽量不要使用
  - `HISTORY_SYSTEM_DEFAULT`：系统默认
- **qos_profile.reliability**：可靠性
  - `RELIABILITY_RELIABLE`：可靠传输
  - `RELIABILITY_BEST_EFFORT`：尽力而为
  - `RELIABILITY_SYSTEM_DEFAULT`：系统默认
- **qos_profile.durability**：持久性（late-join 行为相关）
  - `DURABILITY_VOLATILE`：不为后加入的订阅者补发历史
  - `DURABILITY_TRANSIENT_LOCAL`：缓存并在新订阅者加入时补发历史
  - `DURABILITY_SYSTEM_DEFAULT`：系统默认
- **qos_profile.mps**：messages per second，用于 RTPS 侧内部映射（影响心跳/推进节奏）。常见保持默认 `0` 即可。

---

## 2. QosProfile 是什么

`QosProfile` 是 Segar 使用的一组 QoS 参数。**各字段含义与枚举取值见 §1.3，本节仅补充默认配置。**

- **常用默认（未显式配置时）**：`KEEP_LAST, depth=1, mps=0, RELIABLE, VOLATILE`（可对应“内置 QosProfile”一节的 `QOS_PROFILE_DEFAULT` 理解）。

---

## 3. durability=TRANSIENT_LOCAL 的行为（late-join 补发）

当 Writer 的 `durability` 为 `DURABILITY_TRANSIENT_LOCAL` 时，Segar 的发送端会启用 History 缓存，并在“新 Reader 加入”时补发历史消息（属于 Segar 的内置行为）：

- **写端缓存**：按 `history + depth` 缓存消息
- **订阅者后加入**：会触发补发缓存中的消息

建议用法：

- **消息发送频率很低，但新加入系统的订阅者需要拿到其加入系统前的历史消息**：例如 tf_static、拓扑变更、系统关键状态类 topic
- **高频大流量 topic 禁止开启**：否则缓存与补发会带来大量额外内存与带宽开销（且历史策略/资源限制可能影响实际可缓存量）
-  **zero copy消息原则上禁止使用TRANSIENT_LOCAL**：否则会导致消息生命期被长期甚至无限延长

---

## 4. C++：如何手工配置 QoS

### 4.1 Writer：通过 RoleAttributes 传入 qos_profile

Segar 的 Writer/Reader 都由 `proto::RoleAttributes` 标识，`qos_profile` 是其中的一个字段。

典型写法（构造“ROS 2 sensor 风格 QoS”）：

```cpp
RoleAttributes attr;
attr.set_topic_name(topic_name);
attr.mutable_qos_profile()->CopyFrom(QosProfileConf::CreateQosProfile(
    QosHistoryPolicy::HISTORY_KEEP_LAST, /*depth=*/10,
    QosProfileConf::QOS_MPS_SYSTEM_DEFAULT,
    QosReliabilityPolicy::RELIABILITY_BEST_EFFORT,
    QosDurabilityPolicy::DURABILITY_VOLATILE));
auto writer = node->CreateWriter<MsgT>(attr);
```

如果不设置 `qos_profile`，Segar 会自动填充 `QOS_PROFILE_DEFAULT`。

### 4.2 Reader：通过 ReaderConfig / RoleAttributes 配置 qos_profile 与 pending_queue_size

Reader 侧除了 `qos_profile` 外，还有一个常用参数：

- **pending_queue_size**：接收端的“未处理消息”缓存队列长度

使用 `ReaderConfig` 的常见写法：

```cpp
ReaderConfig cfg;
cfg.topic_name = "topic/name";
cfg.pending_queue_size = 10;
cfg.qos_profile = QosProfileConf::QOS_PROFILE_SENSOR_DATA;  // 可选：设置 QoS
auto reader = node->CreateReader<MsgT>(cfg, callback);
```

使用 `RoleAttributes` 的常见写法（更贴近传输侧的统一入口）：

```cpp
RoleAttributes attr;
attr.set_topic_name("topic/name");
attr.mutable_qos_profile()->CopyFrom(QosProfileConf::QOS_PROFILE_SENSOR_DATA);

// pending_queue_size：callback 未处理消息的缓存长度
// enable_blocker：是否启用 Observe/History 等接口所需的 blocker cache
auto reader = node->CreateReader<MsgT>(attr, callback,
                                      /*pending_queue_size=*/10,
                                      /*enable_blocker=*/false);
```

---

## 5. Python：如何手工配置 QoS（dict 方式）

Segar Python 封装允许在 `CreateWriter/CreateReader` 的 `options` 中传入 `qos_profile`（或直接把 QoS 字段平铺在 options 顶层）。支持键为：

- `history` / `depth` / `mps` / `reliability` / `durability`

其中 `history/reliability/durability` 支持字符串（内部会做大小写归一）：

- `history`：`KEEP_LAST` / `KEEP_ALL` / `SYSTEM_DEFAULT`（也可写 `HISTORY_KEEP_LAST` 等全名）
- `reliability`：`RELIABLE` / `BEST_EFFORT` / `SYSTEM_DEFAULT`
- `durability`：`VOLATILE` / `TRANSIENT_LOCAL` / `SYSTEM_DEFAULT`

Writer 示例：

```python
writer = node.CreateWriter(
    "topic/name",
    MsgType,
    options={
        "qos_depth": 10,  # 兼容参数：会同步到 qos_profile.depth
        "qos_profile": {
            "history": "KEEP_LAST",
            "depth": 10,
            "reliability": "BEST_EFFORT",
            "durability": "VOLATILE",
            "mps": 0,
        },
    },
)
```

Reader 示例：

```python
reader = node.CreateReader(
    "topic/name",
    MsgType,
    callback,
    options={
        "pending_queue_size": 5,
        "qos_profile": {"history": "KEEP_LAST", "depth": 5},
    },
)
```

---

## 6. Segar 内置 QosProfile：你可以直接复用这些预置值

上面 C++/Python 示例中的 `QosProfileConf::QOS_PROFILE_*` 就是一组 **框架内置的预置 profile**（便于统一对齐与复用）。你可以在代码里直接赋值使用，也可以在 `.dag` 中手工填入等价字段。

- **QOS_PROFILE_DEFAULT（默认使用）**
  - `KEEP_LAST, depth=1, mps=0, RELIABLE, VOLATILE`
- **QOS_PROFILE_SENSOR_DATA**
  - `KEEP_LAST, depth=5, mps=0, BEST_EFFORT, VOLATILE`
  - 典型用于 IMU/相机等“更重实时性、允许丢包”的数据流
- **QOS_PROFILE_PARAMETERS**
  - `KEEP_LAST, depth=100, mps=0, RELIABLE, VOLATILE`
- **QOS_PROFILE_SERVICES_DEFAULT**
  - `KEEP_LAST, depth=1000, mps=0, RELIABLE, VOLATILE`
- **QOS_PROFILE_PARAM_EVENT**
  - `KEEP_LAST, depth=100, mps=0, RELIABLE, VOLATILE`
- **QOS_PROFILE_SYSTEM_DEFAULT**
  - `HISTORY_SYSTEM_DEFAULT, depth=0, mps=0, RELIABLE, TRANSIENT_LOCAL`
- **QOS_PROFILE_TF_STATIC**
  - `KEEP_ALL, depth=10, mps=0, RELIABLE, TRANSIENT_LOCAL`
- **QOS_PROFILE_TOPO_CHANGE**
  - `KEEP_ALL, depth=10, mps=0, RELIABLE, TRANSIENT_LOCAL`

推荐：**优先直接选一个预置 profile**；特殊场景再在 `.dag` 或代码里**手工覆盖单个字段**。

---

## 7. `mps` 的作用与高流量场景的 flow control（高级项）

`mps`（messages per second）可以理解为 **RTPS 侧的“发送节奏/推进节奏”提示**：在可靠传输下，它会影响底层在“状态推进/确认/重传”等机制上的推进频率，从而间接影响 **突发流量、队列堆积与端到端延迟**。大多数场景保持 `mps=0`（默认）即可。

什么时候建议显式配置 `mps`（flow control 必要性）：

- **高频/大带宽 topic + RELIABLE**：例如图像/点云/大消息流。若不做 flow control，容易出现发送端/接收端队列快速堆积、延迟抖动变大，甚至在网络拥塞时触发“越重传越拥塞”的放大效应。
- **多 topic 共享链路**：某个大流 topic 的突发会挤占其它关键控制 topic 的带宽与调度窗口，导致关键链路抖动。

配置建议（以“可控、稳定”为目标）：

- **优先判断是否必须 RELIABLE**：允许丢包就用 `BEST_EFFORT`（通常比调参更有效）。
- **必须 RELIABLE 时**：为高流量 topic 设置一个与业务实际速率同量级的 `mps`，用于削峰与稳定推进节奏；其余 topic 保持默认即可（避免全局性改动引入副作用）。
- **配套项**：结合 `pending_queue_size`（回调侧积压）与 `history/depth`（传输侧/缓存侧）一起看；仅调 `mps` 但下游处理能力不足，依然会堆积。

经验上，`mps` 不是“越大越好”的吞吐开关；它更像是在高流量可靠传输场景下，用于 **把不可控的突发变成可控的速率** 的一个手段。

---

## 8. 常见问题

### 8.1 我没有显式配置 QoS，会用什么？

- Writer/Reader 若未携带 `qos_profile`，Transport 会默认填充 `QOS_PROFILE_DEFAULT`。

### 8.2 late-join（后来的接收方希望收到自己启动前的历史消息）拿不到历史怎么办？

- 对需要“新订阅者一启动就拿到最近状态/静态数据”的 topic，考虑把 Writer 设为 `durability=TRANSIENT_LOCAL` 并合理设置 `history/depth`。 `一定要谨慎使用该功能，会给系统负载和性能带来明显压力，非必要不使用`

### 8.3 `pending_queue_size` 和 `depth` 有什么区别？

- `depth`(仅在RTPS模式下，对发送和接收方都生效)：本质上是 history=KEEP_LAST 时的缓存队列深度。
- `pending_queue_size`（在所有通信模式下，对接收方生效）：Reader 回调侧的“未处理消息”缓冲（避免 callback 忙时丢/阻塞）
