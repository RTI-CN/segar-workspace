# Segar Parameter

> **说明**：（可选配置）或（可选阅读）内容都不常用，请酌情了解。
>
> Segar Topic、Segar Service、Segar Action 已介绍过的通用基础知识，本文将不再重述。
>
> Parameter 支持基本类型（int、string 等）和 Protobuf 消息类型，自定义结构通过 `.proto` 文件定义。
>
> **快速流程**：编写 `.proto`（可选）→ 编写 params 配置 → 编写业务代码 → 编译 → 运行 param_server/param_client 示例 → CLI 验证。

---

## 1. 参数类型与配置

### 1.1 基本类型与 Protobuf 类型

- **基本类型**：int、string、double 等，可直接使用
- **Protobuf 类型**：通过 `.proto` 文件定义，workspace 示例统一放在 `src/type_src/segar/proto/` 下，例如 `param_example.proto` 中的 `Header` 消息
- **参数配置文件**：使用 YAML 格式加载参数。默认命名规则是 `${RTI_PARAM_PATH}/<node_name>.yaml -> ${SEGAR_PATH}/config/<node_name>.yaml`
  - 普通 `Node` 默认查找 `${RTI_PARAM_PATH}/<node_name>.yaml`，未命中再查找 `${SEGAR_PATH}/config/<node_name>.yaml`
  - `Component` 使用.dag配置文件中的 `inner_node_name` ，查找规则与普通 `Node` 一致
  - 通过 `CreateNode(..., param_path)` 显式指定参数文件时，绝对路径按原路径读取；相对路径基于 `${SEGAR_PATH}` 解析
- **与 gflags 的区别**：`Parameter` 适合做跨 node / 跨进程查询与设置；`gflags` 只适合当前进程内的全局运行开关


### 1.2 `<node_name>.yaml` 示例

```yaml
param_server:
  segar__parameters:
    p1_int: 1
    p2_string: test
    p3_pb:
      __proto_type__: param.example.Header
      module_name: param_server
      timestamp_sec: 1234.56
      sequence_num: 1
```

### 1.3 命名规范

- 参数名在节点内唯一
- Protobuf 类型需在 YAML 中指定 `__proto_type__` 为完整 proto 类型名（如 `param.example.Header`）

---

## 2. C++ 基本使用示例（非 Component 用法）

示例节选自 `src/param_example/`，拆分为 param_server（本地参数）和 param_client（远程参数）两个示例。

### 2.1 代码说明

- **（必需）** 包含参数 API 头文件 `segar/parameter/segar_parameter_api.h`
- **（必需）** 本地参数需指定 node；远程参数需指定目标 `node_name`（例：`param_server`）
- **（必需）** `rti::segar::Init(argv[0]);` 用于初始化 Segar 系统功能
- **（可选）** `rti::segar::WaitForShutdown();` 用于防止系统主线程退出，使用 Ctrl+C 可退出；如果退出前需要执行用户清理逻辑，可以传入 callback
- **运行顺序**：先运行 param_server，再运行 param_client

### 2.2 服务端（Parameter Server，本地参数）

#### 本地参数 API 说明

- **Segar_Load_Local_Params**：从 YAML 或 dump 文件加载参数到节点
- **Segar_Set_Local_Param**：设置本地参数（支持 int、string、Protobuf 等）
- **Segar_Get_Local_Param**：获取本地参数
- **Segar_List_Local_Params**：列出所有本地参数
- **Segar_Dump_Local_Params**：将本地参数保存到文件

#### 服务端示例

（`src/param_example/param_server/src/param_server.cc`）

```cpp
#include <memory>
#include <string>
#include <vector>

#include "segar/proto/param_example.pb.h"

#include "segar/parameter/segar_parameter_api.h"
#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("param_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using Parameter = rti::segar::Parameter;

  // 加载本地参数
  RETURN_VAL_IF(!Segar_Load_Local_Params(node, "config/param_server.yaml"),
                EXIT_FAILURE);
  AINFO << "Parameter server started successfully";

  AINFO << "=== Testing Local Parameter API ===";

  // 列出本地参数
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "Initial local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // 设置本地参数
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p1_int", 1), EXIT_FAILURE);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p2_string", "test"),
                EXIT_FAILURE);

  param::example::Header header;
  header.set_module_name("param_server");
  header.set_timestamp_sec(1234.56);
  header.set_sequence_num(1);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p3_pb", header), EXIT_FAILURE);

  // 获取本地参数
  int int_val = 0;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p1_int", &int_val), EXIT_FAILURE);
  AINFO << "p1_int: " << int_val;

  std::string str_val;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p2_string", &str_val),
                EXIT_FAILURE);
  AINFO << "p2_string: " << str_val;

  param::example::Header pb_rcv;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p3_pb", &pb_rcv), EXIT_FAILURE);
  AINFO << "p3_pb: " << pb_rcv.DebugString();

  auto header_sp = std::make_shared<param::example::Header>();
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p3_pb", header_sp), EXIT_FAILURE);
  AINFO << "header_sp: " << header_sp->DebugString();

  // 再次列出本地参数
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "After setting, local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // 保存本地参数
  RETURN_VAL_IF(!Segar_Dump_Local_Params(node, "/tmp/param_server.params"),
                EXIT_FAILURE);
  AINFO << "Local parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

### 2.3 客户端（Parameter Client，远程参数）

#### 远程参数 API 说明

- **Segar_Get_Remote_Param**：从指定节点的远程参数服务器获取参数
- **Segar_Set_Remote_Param**：设置远程参数（若支持）
- **Segar_List_Remote_Params**：列出远程节点的所有参数
- **Segar_Dump_Remote_Params**：将远程参数保存到文件
- **Segar_Load_Remote_Params**：将文件中的参数加载到远程节点

#### 客户端示例

（`src/param_example/param_client/src/param_client.cc`）

```cpp
#include <memory>
#include <string>
#include <vector>

#include "segar/proto/param_example.pb.h"

#include "segar/parameter/segar_parameter_api.h"
#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("param_client");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using Parameter = rti::segar::Parameter;

  AINFO << "Parameter client started successfully";
  AINFO << "=== Testing Remote Parameter API ===";

  const std::string node_name = "param_server";

  // 获取远程参数
  int int_val = 0;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p1_int", &int_val),
                EXIT_FAILURE);
  AINFO << "p1_int: " << int_val;

  std::string str_val;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p2_string", &str_val),
                EXIT_FAILURE);
  AINFO << "p2_string: " << str_val;

  param::example::Header pb_rcv;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p3_pb", &pb_rcv),
                EXIT_FAILURE);
  AINFO << "p3_pb: " << pb_rcv.DebugString();

  auto header_sp = std::make_shared<param::example::Header>();
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p3_pb", header_sp),
                EXIT_FAILURE);
  AINFO << "header_sp: " << header_sp->DebugString();

  // 加载远程参数
  RETURN_VAL_IF(
      !Segar_Load_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);

  // 列出远程参数
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Remote_Params(node_name, &parameter_list),
                EXIT_FAILURE);
  AINFO << "Remote parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // 保存远程参数
  RETURN_VAL_IF(
      !Segar_Dump_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);
  AINFO << "Remote parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

---

## 3. CLI 调试

`segar param` 命令可用于查询和设置 Parameter（详细功能请参见 Segar CLI 文档）：

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
