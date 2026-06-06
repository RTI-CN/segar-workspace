# 1. 数采支持

## 1.1 动态服务发现（Topo）接口

### 1.1.1 获取当前 Topo 快照

- **用途**：拉取当前拓扑快照（例如 topic 名、消息类型等），便于数采侧做 topic 自动识别/订阅策略生成。
- **核心输出**：`RoleAttributes` 中的 `topic_name`、`message_type` 等字段。

```cpp
// API（示意）
class TopicManager {
 public:
  void GetWriters(RoleAttrVec* writers);
};

// proto（示意）
message RoleAttributes {
  // ...
  optional string topic_name = 6;
  optional string message_type = 8;
  // ...
}

// 使用示例（示意）
auto topic_manager = rti::segar::TopologyManager::Instance()->topic_manager();
std::vector<rti::segar::proto::RoleAttributes> role_attr_vec;
topic_manager->GetWriters(&role_attr_vec);
```

### 1.1.2 持续监控 Topo 变化

- **用途**：监听拓扑 join/leave 等变化事件，实时增量更新 topic 列表或做告警/统计。
- **注意**：
  - 回调可能来自内部线程，回调里尽量避免阻塞；需要跨线程处理时建议丢到队列/线程池。
  - 监听器生命周期要和 `ChangeConnection` 绑定，销毁/退订时记得 `RemoveChangeListener`。

```cpp
// API（示意）
class TopicManager {
 public:
  using ChangeFunc = std::function<void(const ChangeMsg&)>;
  ChangeConnection AddChangeListener(const ChangeFunc& func);  // 添加监听器
  void RemoveChangeListener(const ChangeConnection& conn);     // 移除监听器
};

// proto（精简版）
message ChangeMsg {
  optional uint64 timestamp = 1;
  optional ChangeType change_type = 2;
  optional OperateType operate_type = 3;
  optional RoleType role_type = 4;
  optional RoleAttributes role_attr = 5;
};

enum ChangeType {
  CHANGE_NODE = 1;
  CHANGE_TOPIC = 2;
  CHANGE_SERVICE = 3;
  CHANGE_PARTICIPANT = 4;
  CHANGE_ACTION = 5;
};

enum OperateType {
  OPT_JOIN = 1;
  OPT_LEAVE = 2;
};

// 使用示例（示意）
auto topic_manager = rti::segar::TopologyManager::Instance()->topic_manager();
auto topology_callback =
    [this](const rti::segar::proto::ChangeMsg& change_msg) { /* do something */ };
change_conn_ = topic_manager->AddChangeListener(topology_callback);

// 退出/析构时退订
topic_manager->RemoveChangeListener(change_conn_);
```

## 1.2 收发原始字节流支持

当你希望**绕过强类型消息定义**、直接收发 payload（例如做录包/转发/桥接/可视化工具）时，可以在创建 reader/writer 时将消息类型指定为 `rti::segar::message::IdlRawMessage`。

- **说明**：`IdlRawMessage` 内部携带的 `raw_data()` 即为该消息的**原始字节流**（`std::vector<uint8_t>` / bytes）。

## 1.3 类型系统支持

提供类型系统及相关工具，目前提供以下接口供使用。

相关头文件（随依赖安装）：

- `msg_tool/msg/type_registry.h`

### 1.3.1 获取类型信息：`msg_tool::msg::get_type`

按 `type_name` 查询已注册类型，返回该类型的描述信息（未找到返回 `nullptr`）。

```cpp
// API
const msg_tool::msg::TypeItem* msg_tool::msg::get_type(const std::string& type_name);

// 使用示例（示意）
#include "msg_tool/msg/type_registry.h"

// 1) 查询类型信息
auto type_item = msg_tool::msg::get_type("your_dds_type_name");
if (!type_item) {
  // type 未注册/未找到
  return;
}

// 2) 创建 FastDDS TypeSupport（如需在 DDS 层做注册/反序列化）
auto ts = std::shared_ptr<eprosima::fastdds::dds::TopicDataType>(type_item->create_ts_func());

// 3) 将字节流解析成对象并转 json（以 IdlRawMessage 为例，示意）
void Callback(std::shared_ptr<rti::segar::message::IdlRawMessage> msg) {
  // raw_data 即该消息携带的原始字节流（bytes）
  const auto& raw_data = msg->raw_data();
  void* data_ptr = type_item->serialize_func(raw_data.data(), raw_data.size());
  auto json_data = type_item->to_json_fun(data_ptr);

  // data_ptr 释放
  ts->deleteData(data_ptr);
}
```

返回结构：`msg_tool::msg::TypeItem`

```cpp
struct TypeItem {
  std::string type_name;
  std::function<eprosima::fastdds::dds::TopicDataType*()> create_ts_func;
  std::function<nlohmann::json(void* data)> to_json_fun;
  std::function<void*(const void* serialized_data, size_t size)> serialize_func;
  std::string schema;
  std::vector<std::string> schema_dependencies;
};
```

字段说明：

- **`type_name`**：类型名（用于注册/查询的 key）。
- **`create_ts_func`**：创建 FastDDS 的 `TopicDataType`（TypeSupport）对象的工厂函数，用于在 DDS 层注册/反序列化该类型。
- **`to_json_fun`**：将已反序列化后的对象转换为 `nlohmann::json`，便于可视化/调试。
  - 约定入参 `void* data` 指向该类型的实例内存。
- **`serialize_func`**：将“序列化后的字节流”解析为“该类型的内存对象”的函数（用于把外部字节流还原成对象以便进一步处理）。
  - 入参：`serialized_data` / `size` 为字节缓冲区与长度；返回值为新建对象指针（由调用方按约定释放）。
- **`schema`**：该类型的 `.msg` schema 字符串。
  - **仅对 msg 类型有效**；对 `srv/action/idl` 类型可能为空。
- **`schema_dependencies`**：该类型依赖的“嵌套 msg 类型”的 `dds_type_name` 列表。
  - **仅对 msg 类型有效**；用于组合完整 schema 集合（避免重复）。

### 1.3.2 获取 schema 信息：`msg_tool::msg::get_schema_map`

返回**用于 MCAP 的 schema 映射列表**：`(type_name, schema)`。会从 `type_name` 出发，按 `schema_dependencies` **广度优先（BFS）**遍历依赖，输出**有序且不重复**的 schema 列表，便于一次性携带主类型及其嵌套依赖的全部 schema。

```cpp
// API
std::vector<std::pair<std::string, std::string>>
msg_tool::msg::get_schema_map(const std::string& type_name);

// 使用示例（示意）
#include "msg_tool/msg/type_registry.h"

auto schema_map = msg_tool::msg::get_schema_map("your_dds_type_name");
for (const auto& [type_name, schema] : schema_map) {
  // type_name: 该条 schema 对应的类型名
  // schema:    该类型的 .msg schema 字符串
}
```

## 2. 仿真支持

下面的接口用于在仿真场景下启用/设置/获取时钟，以便回放、加速、暂停等能力实现。

### 2.1 使能仿真时钟（Mock Clock）

#### 2.1.1 通过配置使能

```proto
run_mode_conf {
  run_mode: MODE_REALITY
  clock_mode: MODE_MOCK
}
```

#### 2.1.2 通过 API 使能

```cpp
// API（示意）
class Clock {
 public:
  static void SetMode(ClockMode mode);
};

// 使用示例
#include "segar/time/clock.h"
rti::segar::Clock::SetMode(rti::segar::ClockMode::MODE_MOCK);
```

### 2.2 设置仿真时间戳

#### 2.2.1 通过 API 设置

```cpp
// API（示意）
class Clock {
 public:
  static void SetNow(const Time& now);
};

// 使用示例
#include "segar/time/clock.h"
rti::segar::Clock::SetNow(rti::segar::Time(1));
```

#### 2.2.2 通过发布 Topic 设置

- **说明** beta功能，如有需要需要进一步测试再正式发布：

```cpp
auto clock_node = std::make_shared<Node>("node_name");
auto clock_writer =
    clock_node->CreateWriter<rti::segar::proto::Clock>(kClockTopic);

rti::segar::proto::Clock now_clock;
now_clock.set_clock(static_cast<uint64_t>(666));
clock_writer->Send(now_clock);
```

### 2.3 获取仿真时间戳

```cpp
// API
class Clock {
 public:
  static Time Now();
  static double NowInSeconds();
};

// 使用示例
#include "segar/time/clock.h"
auto t = rti::segar::Clock::Now();
auto ts = rti::segar::Clock::NowInSeconds();
```
