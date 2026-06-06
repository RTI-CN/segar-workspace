# Segar Lifecycle

> **说明**：Segar Lifecycle 用于在运行期动态控制 Node 或 Component 的业务是否处于激活状态。本文只介绍基本概念、用途、使用方法与注意事项，不展开实现原理。
>
> **快速流程**：创建 Node 或 Component → 默认进入 `Active` → 需要暂停业务时调用 `Deactivate()` → 需要恢复业务时调用 `Activate()` → 通过 `GetState()` 查看当前状态

---

## 1. 核心概念速览

- **Lifecycle**：Segar 提供的轻量运行态控制能力，用于在 `Active` 与 `Inactive` 之间切换
- **Active**：激活状态。Topic / Service / Action 可以正常承载业务，Component 的 `Proc()` 会正常执行
- **Inactive**：非激活状态。业务暂停，不再继续处理 Topic / Service / Action 的业务逻辑，Component 的 `Proc()` 不再执行
- **Node Lifecycle**：直接作用在 `Node` 上，适用于普通 Topic / Service / Action 编程模式
- **Component Lifecycle**：作用在 `Component` / `TimerComponent` / `SyncComponent` 上，接口与 Node 保持一致

---

## 2. 适用场景

- **临时暂停业务**：例如摘流、短暂停机排障、切换运行模式
- **快速恢复业务**：无需重建对象，恢复后可以继续工作
- **统一控制组件运行态**：对 Node 和 Component 使用同一组激活/非激活接口

---

## 3. 状态与接口

Segar Lifecycle 只包含两个稳定状态：

- **Active**
- **Inactive**

常用接口如下：

- `Activate()`：切换到 `Active`
- `Deactivate()`：切换到 `Inactive`
- `GetState()`：获取当前状态

**说明**：

- `CreateNode()` 创建出来的 Node 默认处于 `Active`
- 如果对象已经处于目标状态，再次调用 `Activate()` 或 `Deactivate()` 会返回 `false`

---

## 4. Node 使用方法

### 4.1 基本用法

Node 默认创建后就是 `Active`，可以正常创建 Writer / Reader / Service / Client / Action。

```cpp
#include "example/msg/String.hpp"
#include "segar/segar.h"

int main(int argc, char* argv[]) {
  if (!rti::segar::Init(argv[0])) {
    return -1;
  }

  auto node = rti::segar::CreateNode("lifecycle_node");
  if (!node) {
    return -1;
  }

  auto writer = node->CreateWriter<example::msg::String>("/topic/lifecycle");
  if (!writer) {
    return -1;
  }

  node->Deactivate();  // 暂停业务
  node->Activate();    // 恢复业务

  return 0;
}
```

### 4.2 行为说明

- `Active` 时：Topic / Service / Action 可以正常处理业务
- `Inactive` 时：Topic / Service / Action 的业务处理暂停
- `Parameter` 不受 Lifecycle 影响，仍可继续使用

---

## 5. Component 使用方法

`Component`、`TimerComponent`、`SyncComponent` 同样支持：

- `Activate()`
- `Deactivate()`
- `GetState()`

使用方式与 Node 保持一致：

```cpp
auto component = std::make_shared<MyComponent>();
component->Initialize(config);

component->Deactivate();  // 暂停业务 Proc
component->Activate();    // 恢复业务 Proc
```

### 5.1 作用范围

- `Component`：`Inactive` 时不再进入业务 `Proc(...)`
- `TimerComponent`：`Inactive` 时不再进入业务 `Proc()`
- `SyncComponent`：`Inactive` 时不再进入同步后的业务 `Proc(...)`

---

## 6. 示例位置

工作区中提供了两个对应示例：

- `src/lifecycle_example/node_lifecycle/`：演示 Node 的 `Activate()` / `Deactivate()`
- `src/lifecycle_example/component_lifecycle/`：演示组件体系（当前以 `TimerComponent` 为例）的 `Activate()` / `Deactivate()`

编译后可通过各自目录下的 `scripts/launch.sh` 直接运行。

---

## 7. 注意事项

- Lifecycle 只负责控制业务的激活与非激活，不用于原地重新配置对象
- 如果需要替换配置，建议销毁旧对象后重新创建新对象
- Node 默认创建后就是 `Active`，这有利于兼容现有大多数使用方式
- `Inactive` 并不等同于删除对象；恢复时应调用 `Activate()`
- `Parameter` 不受 Lifecycle 控制，适合在业务暂停期间继续做参数查看和调整
