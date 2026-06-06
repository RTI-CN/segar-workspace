# Segar Python 工程示例（src_python）

---

## 1. 文档入口

- [Segar Python 用户手册](Segar_Python_User_Guide.md)
- [Python Topic 使用入门](Segar_Python_Topic.md)
- [Python Service 使用入门](Segar_Python_Service.md)
- [Python Action 使用入门](Segar_Python_Action.md)
- [Python Parameter 使用入门](Segar_Python_Parameter.md)
- [Python API 参考手册](Segar_Python_Api_Reference.md)

---

## 2. 目录布局

`src_python` 按 `src` 同款“工程模块”布局组织，构建后安装到：

- `build_x86/output/src_python/topic_example/...`
- `build_x86/output/src_python/usr_msg_topic_example/...`
- `build_x86/output/src_python/service_example/...`
- `build_x86/output/src_python/action_example/...`
- `build_x86/output/src_python/param_example/...`

---

## 3. 构建

```bash
cd /home/simon/segar/segar-workspace
./scripts/build_x86.sh
```

---

## 4. 运行（示例）

### Topic

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```

再开一个终端：

```bash
cd build_x86/output/src_python/topic_example/topic_talker
./scripts/launch.sh
```

### usr_msg TypeCoverage Topic

```bash
cd build_x86/output/src_python/usr_msg_topic_example/type_coverage_listener
./scripts/launch.sh
```

再开一个终端：

```bash
cd build_x86/output/src_python/usr_msg_topic_example/type_coverage_talker
./scripts/launch.sh
```

### Service

```bash
cd build_x86/output/src_python/service_example/service_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/service_example/service_client_sync
./scripts/launch.sh
```

### Action

```bash
cd build_x86/output/src_python/action_example/action_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/action_example/action_client_async
./scripts/launch.sh
```

### Parameter

```bash
cd build_x86/output/src_python/param_example/param_server
./scripts/launch.sh
```

```bash
cd build_x86/output/src_python/param_example/param_client
./scripts/launch.sh
```

`launch.sh` 启动时会自动切换到当前模块根目录，因此也可以直接在 `scripts/` 目录中执行 `bash launch.sh`。

---

## 5. 当前模块映射

- `topic_example/topic_talker`
- `topic_example/topic_listener`
- `usr_msg_topic_example/type_coverage_talker`
- `usr_msg_topic_example/type_coverage_listener`
- `service_example/service_server`
- `service_example/service_client_sync`
- `service_example/service_client_async`
- `action_example/action_server`
- `action_example/action_client_sync`
- `action_example/action_client_async`
- `param_example/param_server`
- `param_example/param_client`

---

## 6. 运行环境说明

`output/segar_setup.bash` 统一自动设置以下公共路径，不需要每个 Python 示例脚本`launch.sh`手工拼接：

- `LD_LIBRARY_PATH`
- `SEGAR_PATH`
- `PYTHONPATH`

`usr_msg` 这类通用消息库会安装自己的 Python 消息包，Python 示例只需要导入对应消息类型，例如：

```python
from test_msgs.msg import TypeCoverage
```

当前 Python IDL 只支持 topic；Python IDL service/action 暂时不可用。
