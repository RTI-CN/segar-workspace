# Segar CLI

> **说明**：为方便用户使用，本工具的使用方式及输出风格与 ROS 2 基本一致，输出信息做了部分扩充，并支持命令补全。
>
> **环境准备**：需先完成环境变量设置，才可使用如下命令。在编译产物的 output 目录下执行 `source segar_setup.bash`，或参见项目 README 的运行说明。

---

## 1. node 子命令

```bash
segar node
usage: segar node [-h] {list,info}...

Various node related utilities

commands:
  list                        List all nodes
  info <NodeName>             Node information
```

---

## 2. topic 子命令

```bash
segar topic
usage: segar topic [-h] {list,info,bw,hz,type,echo,pub,qs} [-h] ...

Various topic related utilities

commands:
  list                        List all topics
  info <TopicName>            Print information about a topic
  bw <TopicName>              Display bandwidth of a topic
  hz <TopicName>              Display publishing rate of topic
  type <TopicName>            Display type of topic
  echo <TopicName>            Echo messages from a topic
  pub <TopicName> <MessageType> [values]
  qs <TopicName> [<Size>] [--node <NodeName>]
```

`pub` 用于向 topic 发布 IDL 消息：

```bash
segar topic pub [-r RATE] [-t TIMES] <TopicName> <MessageType> [values]
```

参数说明：

| 参数 | 说明 |
|------|------|
| `-r, --rate RATE` | 发布频率，单位 Hz，默认 `1` |
| `-t, --times TIMES` | 发布指定次数后退出；不指定时持续发布 |
| `TopicName` | topic 名称，例如 `/topic/chatter` |
| `MessageType` | 消息类型，例如 `example/msg/String` 或 `example/msg/String.msg` |
| `values` | YAML/JSON mapping 格式的字段值；不指定时使用默认值 |

示例：

```bash
segar topic pub -r 10 -t 100 /topic/chatter example/msg/String '{data: "hello"}'
```

`qs` 用于查询或动态设置 callback Reader 的 `pending_queue_size`：

```bash
# 查询 topic 对应 Reader 当前的 pending_queue_size
segar topic qs <TopicName> [--node <NodeName>]

# 设置 topic 对应 Reader 的 pending_queue_size，Size 必须大于 0
segar topic qs <TopicName> <Size> [--node <NodeName>]
```

参数说明：

| 参数 | 说明 |
|------|------|
| `TopicName` | topic 名称，例如 `/topic/chatter` |
| `Size` | 新的 `pending_queue_size`；省略时为查询模式 |
| `--node NodeName` | 指定目标 Node；查询时可省略并输出匹配的 Reader，设置时若存在多个匹配 Reader 则必须指定 |

示例：

```bash
segar topic qs /topic/chatter
segar topic qs /topic/chatter --node listener
segar topic qs /topic/chatter 32 --node listener
```

说明：`qs` 目前用于普通 `Node::CreateReader` callback Reader 的运行期队列调整；不用于多消息 Reader 或 DAG Component 的配置替代。

---

## 3. service 子命令

```bash
segar service
usage: segar service [-h] {list,type,info} ...

Various service related utilities

commands:
  list                        List all services
  type <ServiceName>          Display .srv type for a service
  info <ServiceName>          Display info of service
```

---

## 4. action 子命令

```bash
segar action
usage: segar action [-h] {list,type,info} ...

Action related utilities

commands:
  list                        List all actions
  type <ActionName>           Display .action type for an action
  info <ActionName>           Display info of action
```

---

## 5. param 子命令

```bash
segar param
usage: segar param [-h] {list,get,set,dump,load} ...

Various parameter related utilities

commands:
  list <NodeName>                     List parameters
  get  <NodeName> <ParamName>         Get parameter value
  set  <NodeName> <ParamName> <Value> Set parameter value
  dump <NodeName> <FileName>           Dump parameters to YAML
  load <NodeName> <FileName>          Load parameters from YAML
```
