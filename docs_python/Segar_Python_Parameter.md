# Segar Python Parameter 使用入门

---

## 1. 接口位置

参数 API 位于：

- `from segar.python import parameter`

命名与 C++ 侧保持 Segar 前缀风格（`Segar_*`）。

---

## 2. 本地参数（Server 侧）

对应工程模块：

- `src_python/param_example/param_server/src/param_server.py`

常用接口：

- `Segar_Load_Local_Params(node, yaml_file)`
- `Segar_Set_Local_Param(node, param_name, value)`
- `Segar_Get_Local_Param(node, param_name, out_value, value_type)`
- `Segar_List_Local_Params(node, out_list)`
- `Segar_Dump_Local_Params(node, yaml_file)`

---

## 3. 远程参数（Client 侧）

对应工程模块：

- `src_python/param_example/param_client/src/param_client.py`

常用接口：

- `Segar_Get_Remote_Param(service_node_name, param_name, out_value, value_type)`
- `Segar_Set_Remote_Param(service_node_name, param_name, value)`
- `Segar_List_Remote_Params(service_node_name, out_list)`
- `Segar_Load_Remote_Params(service_node_name, yaml_file)`
- `Segar_Dump_Remote_Params(service_node_name, yaml_file)`

---

## 4. OutValue 用法

`Segar_Get_*_Param` 使用输出参数风格：

```python
from segar.python import parameter

out_int = parameter.OutValue()
ok = parameter.Segar_Get_Remote_Param("param_server", "p1_int", out_int, int)
if ok:
    print(out_int.value)
```

---

## 5. YAML 配置示例

`src_python/param_example/param_server/config/param_server.yaml`：

```yaml
param_server:
  segar__parameters:
    p1_int: 1
    p2_string: test
```

---

## 6. 运行方式

```bash
cd build_x86/output/src_python/param_example/param_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/param_example/param_client
./scripts/launch.sh
```
