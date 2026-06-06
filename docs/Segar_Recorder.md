# Segar Bag

`segar bag` 用于录制、回放和查看 topic 数据，基于 MCAP 格式。本文档介绍 `record`、`play`、`info` 三个子命令的用法。

> 为了方便ros2用户迁移，`segar bag`命令风格和 `ros2 bag`基本一致

---

## 一、record（录制）

将指定 topic 的数据录制到 MCAP 文件。

### 命令格式

```bash
segar bag record [options]
```

### 必选条件（二选一）

| 选项 | 说明 |
|------|------|
| `-a, --all` | 录制所有 topics |
| `-t, --topics <topic>...` | 录制指定 topic（可指定多个）。支持 `topic:=type` 格式同时指定消息类型（用于 ROS2、无 publisher 时预订阅） |

### 可选参数

| 选项 | 说明 |
|------|------|
| `-o, --output <dir>` | 输出目录名，默认 `segar_bag_{时间戳}/` |
| `-i, --segment-interval <秒>` | 按时间分段，每 N 秒生成新文件 |
| `-m, --segment-size <MB>` | 按大小分段，每 N MB 生成新文件 |
| `-h, --help` | 显示帮助 |

### 输出说明

录制结果输出到目录，目录内文件命名为 `{目录名}_0.mcap`、`{目录名}_1.mcap` 等。

### 示例

```bash
# 录制指定 topic，输出到 my_bag/
segar bag record -t rt/topic/chatter_idl -o my_bag

# 录制多个 topics
segar bag record -t /topic_a /topic_b -o multi_bag

# 录制所有 topics，使用默认输出目录
segar bag record -a

# 用 topic:=type 指定消息类型（主要用于录制 ROS2 topic）
segar bag record -t rt/chatter:=std_msgs::msg::String -o ros2_bag
# 按 100MB 分段
segar bag record -t /chatter -o big_bag -m 100
```

### topic:=type 格式说明

录制 ROS2 topic 时，使用 `-t topic:=type` 同时指定 topic 名与消息类型，例如：

- `-t rt/chatter:=std_msgs::msg::String`：录制 ROS2 `/chatter` 桥接 topic
---

## 二、play（回放）

回放录制文件或目录中的 MCAP 数据。

### 命令格式

```bash
segar bag play <bag_path> [options]
```

### 必选参数

| 参数 | 说明 |
|------|------|
| `bag_path` | 录制文件路径或目录路径（位置参数） |

支持单文件（如 `my_bag_0.mcap`）或目录（如 `my_bag/`，自动展开其中的 `*_0.mcap`、`*_1.mcap` 等）。

### 可选参数

| 选项 | 说明 |
|------|------|
| `-a, --all` | 回放所有 topics（默认行为） |
| `-t, --topics <topic>...` | 仅回放指定 topics |
| `-k, --black-topic <name>` | 排除指定 topics |
| `-l, --loop` | 循环回放 |
| `-r, --rate <倍率>` | 回放倍率，默认 1.0 |
| `-b, --begin <时间>` | 开始时间（如 2018-07-01-00:00:00） |
| `-e, --end <时间>` | 结束时间 |
| `-s, --start <秒>` | 从第 N 秒开始 |
| `-d, --delay <秒>` | 开始前延时秒数 |
| `-p, --preload <秒>` | 预加载时长 |
| `-m, --remap <old:=new>` | Topic 重映射 |
| `-h, --help` | 显示帮助 |

### 示例

```bash
# 回放目录
segar bag play my_bag

# 回放单文件
segar bag play my_bag/my_bag_0.mcap

# 仅回放指定 topic，循环，2 倍速
segar bag play my_bag -t rt/topic/chatter_idl -l -r 2.0

# Topic 重映射：将 /chatter 发布到 /remapped_chatter
segar bag play my_bag -m /chatter:=/remapped_chatter
```

---

## 三、info（查看信息）

查看录制文件或目录的元数据和统计信息。

### 命令格式

```bash
segar bag info <path>
```

### 参数

| 参数 | 说明 |
|------|------|
| `path` | 录制文件或目录路径 |

传入目录时，会汇总目录内所有 `*_N.mcap` 文件的统计信息。

### 输出说明

- `record_file`：路径
- `version`：版本
- `duration`：时长（秒）
- `begin_time` / `end_time`：起止时间
- `size`：文件大小
- `is_complete`：是否完整
- `message_number`：消息总数
- `topic_number`：topic 数量
- `topic_info`：各 topic 的消息数与类型

### 示例

```bash
# 查看单文件
segar bag info my_bag_0.mcap

# 查看目录（汇总统计）
segar bag info my_bag
```

---

## 四、完整示例：录制与回放 ROS2 demo_nodes_cpp 话题

> segar bag 录制的 MCAP 属于标准格式，可使用 [Foxglove Studio](https://foxglove.dev/) 和 `ros2 bag` 进行播放与包内容查看。

下面以 ROS 2 自带的 `demo_nodes_cpp` 为例，演示如何用 segar bag 录制 ROS2 topic，并分别用 segar bag 和 ros2 bag 回放。

### 1. 录制

**终端 1**：启动 ROS2 talker
```bash
ros2 run demo_nodes_cpp talker
```

**终端 2**：用 segar bag 录制 `/chatter`（通过桥接后 topic 名为 `rt/chatter`）
```bash
segar bag record -a -t rt/chatter:=std_msgs::msg::String -o ros2_demo_bag
```
录制完成后按 Ctrl+C 停止，输出目录为 `ros2_demo_bag/`，文件为 `ros2_demo_bag_0.mcap`。

### 2. 用 segar bag 回放

```bash
segar bag play ros2_demo_bag
# 或指定单文件
segar bag play ros2_demo_bag/ros2_demo_bag_0.mcap
```

**终端 3**（可选）：启动 listener 接收回放数据
```bash
ros2 run demo_nodes_cpp listener
```

### 3. 用 ros2 bag 回放

segar bag 生成的 MCAP 文件可直接用 ros2 bag 回放。ROS2 端 topic 名为 `/chatter`，需将录制的 `rt/chatter` 重映射：

```bash
ros2 bag play ros2_demo_bag/ros2_demo_bag_0.mcap --remap rt/chatter:=/chatter
```

> 说明：`rt/chatter` 为 segar 桥接后的 topic 名，`/chatter` 为 ROS2 原生 topic 名。通过 `--remap` 重映射后，`ros2 run demo_nodes_cpp listener` 可正常接收。
