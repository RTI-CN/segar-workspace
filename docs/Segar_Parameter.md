# Getting Started with Parameter

> **Note**: The contents of (optional configuration) or (optional reading) are not commonly used, please understand as appropriate.
>
> The general basic knowledge of Segar Topic, Service, and Action has been introduced in the introduction and will not be repeated in this article.
>
> Parameter supports basic types (int, string, etc.) and Protobuf message types, and custom structures are defined through `.proto` files.
>
> **Quick process**: Write `.proto` (optional) → Write params configuration → Write business code → Compile → Run param_server/param_client example → CLI verification.

---

## 1. Parameter type and configuration

### 1.1 Basic types and Protobuf types

- **Basic types**: int, string, double, etc., can be used directly
- **Protobuf type**: defined by `.proto` file, such as `Header` message in `param_example.proto`
- **Configuration file**: Use YAML format to load parameters, such as `config/params.yaml`

### 1.2 params.yaml example

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

### 1.3 Naming convention

- Parameter names are unique within the node
- Protobuf type needs to be specified in YAML. `__proto_type__` is the complete proto type name (such as `param.example.Header`)

---

## 2. Basic usage examples of C++ (without Components)

The example is excerpted from `src/param_example/`, split into two examples: param_server (local parameters) and param_client (remote parameters).

### 2.1 Code description

- **(required)** Contains parameter API header `segar/parameter/segar_parameter_api.h`
- **(required)** The local parameter needs to specify node; the remote parameter needs to specify the target `node_name` (example: `param_server`)
- **(required)** `rti::segar::Init(argv[0]);` is used to initialize Segar system functions
- **(optional)** `rti::segar::WaitForShutdown();` is used to prevent the system main thread from exiting. Use Ctrl+C to exit.
- **Running order**: Run param_server first, then param_client

### 2.2 Server (Parameter Server, local parameters)

#### Local parameter API description

- **Segar_Load_Local_Params**: Load parameters from YAML or dump file to node
- **Segar_Set_Local_Param**: Set local parameters (supports int, string, Protobuf, etc.)
- **Segar_Get_Local_Param**: Get local parameters
- **Segar_List_Local_Params**: List all local parameters
- **Segar_Dump_Local_Params**: Save local parameters to a file

#### Server example

(`src/param_example/param_server/src/param_server.cc`)

```cpp
#include <memory>
#include <string>
#include <vector>

#include "param_example.pb.h"

#include "segar/parameter/segar_parameter_api.h"
#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("param_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using Parameter = rti::segar::Parameter;

//Load local parameters
  RETURN_VAL_IF(!Segar_Load_Local_Params(node, "config/params.yaml"),
                EXIT_FAILURE);
  AINFO << "Parameter server started successfully";

  AINFO << "=== Testing Local Parameter API ===";

// List local parameters
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "Initial local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

//Set local parameters
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p1_int", 1), EXIT_FAILURE);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p2_string", "test"),
                EXIT_FAILURE);

  param::example::Header header;
  header.set_module_name("param_server");
  header.set_timestamp_sec(1234.56);
  header.set_sequence_num(1);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p3_pb", header), EXIT_FAILURE);

// Get local parameters
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

// List local parameters again
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "After setting, local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

//Save local parameters
  RETURN_VAL_IF(!Segar_Dump_Local_Params(node, "/tmp/param_server.params"),
                EXIT_FAILURE);
  AINFO << "Local parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

### 2.3 Client (Parameter Client, remote parameters)

#### Remote parameter API description

- **Segar_Get_Remote_Param**: Get parameters from the remote parameter server of the specified node
- **Segar_Set_Remote_Param**: Set remote parameters (if supported)
- **Segar_List_Remote_Params**: List all parameters of the remote node
- **Segar_Dump_Remote_Params**: Save remote parameters to file
- **Segar_Load_Remote_Params**: Load parameters in the file to the remote node

#### Client example

(`src/param_example/param_client/src/param_client.cc`)

```cpp
#include <memory>
#include <string>
#include <vector>

#include "param_example.pb.h"

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

// Get remote parameters
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

//Load remote parameters
  RETURN_VAL_IF(
      !Segar_Load_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);

// List remote parameters
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Remote_Params(node_name, &parameter_list),
                EXIT_FAILURE);
  AINFO << "Remote parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

//Save remote parameters
  RETURN_VAL_IF(
      !Segar_Dump_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);
  AINFO << "Remote parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
```

---

## 3. CLI Debugging

The `segar param` command can be used to query and set Parameter (for detailed functions, please refer to the Segar CLI Getting Started Document):

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
