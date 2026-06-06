# SegarHeader Configuration

> **说明**：SegarHeader 是否自动注入，统一由 `type_src/transport_messages` 清单控制。

---

## 1. 背景与目标

- `SegarHeader` 用于承载会话/序列号/时间戳等传输侧元信息
- 只有通过 Segar API 收发的消息，才需要自动添加 `SegarHeader`
- 为了避免业务消息定义与注入策略耦合，改为“配置驱动”模式

---

## 2. 配置入口

在每个消息根目录（`TYPE_SRC_DIR`）下创建文件：

```text
type_src/transport_messages
```

- 一行一条规则
- 路径相对 `type_src` 目录
- 支持注释行（以 `#` 开头）和空行

---

## 3. 规则格式

### 3.1 文件级规则（精确匹配）

```text
example/msg/Image.msg
example/srv/SetCameraInfo.srv
example/action/LookUpTransform.action
```

### 3.2 通配符规则

```text
example/msg/Test*
example/action/*
```

说明：匹配使用 `fnmatch` 语义（`*`, `?`, `[]`），其中 `目录/*` 可匹配该目录及其子目录中的接口定义。

---

## 4. 示例配置

```text
# 只对这三类接口注入 SegarHeader
test_msgs/action/*
test_msgs/msg/Test*
test_msgs/srv/*
```

含义：

- `test_msgs/action/*`：该目录及子目录下所有 `.action` 注入
- `test_msgs/msg/Test*`：只匹配 `Test` 开头的消息定义
- `test_msgs/srv/*`：该目录及子目录下所有 `.srv` 注入

---

## 5. 常见问题

### 5.1 配置了但没生效

- 检查 `transport_messages` 是否放在 `TYPE_SRC_DIR` 根目录
- 检查路径是否是“相对 `type_src`”而不是绝对路径
- 检查构建是否使用了新版本 `msg_tool`
- 清理旧构建产物后重新生成

### 5.2 规则写了目录但未匹配

- 目录本身不是合法规则，请使用通配符写法：`pkg/msg/*`、`pkg/srv/*`、`pkg/action/*`
- 避免规则前后有多余空格
