# Getting started with Segar Python Parameter

---

## 1. Interface location

The parameter API is located at:

- `from segar.python import parameter`

Naming maintains the Segar prefix style (`Segar_*`) with the C++ side.

---

## 2. Local parameters (Server side)

Corresponding engineering module:

- `src_python/param_example/param_server/src/param_server.py`

Commonly used interfaces:

- `Segar_Load_Local_Params(node, yaml_file)`
- `Segar_Set_Local_Param(node, param_name, value)`
- `Segar_Get_Local_Param(node, param_name, out_value, value_type)`
- `Segar_List_Local_Params(node, out_list)`
- `Segar_Dump_Local_Params(node, yaml_file)`

---

## 3. Remote parameters (Client side)

Corresponding engineering module:

- `src_python/param_example/param_client/src/param_client.py`

Commonly used interfaces:

- `Segar_Get_Remote_Param(service_node_name, param_name, out_value, value_type)`
- `Segar_Set_Remote_Param(service_node_name, param_name, value)`
- `Segar_List_Remote_Params(service_node_name, out_list)`
- `Segar_Load_Remote_Params(service_node_name, yaml_file)`
- `Segar_Dump_Remote_Params(service_node_name, yaml_file)`

---

## 4. OutValue usage

`Segar_Get_*_Param` uses output parameter style:

```python
from segar.python import parameter

out_int = parameter.OutValue()
ok = parameter.Segar_Get_Remote_Param("param_server", "p1_int", out_int, int)
if ok:
    print(out_int.value)
```
---

## 5. YAML configuration example

`src_python/param_example/param_server/config/params.yaml`:

```yaml
param_server:
  segar__parameters:
    p1_int: 1
    p2_string: test
```
---

## 6. Operation mode

```bash
cd build_x86/output/src_python/param_example/param_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/param_example/param_client
./scripts/launch.sh
```
