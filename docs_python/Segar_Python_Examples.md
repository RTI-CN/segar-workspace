# Segar Python project example (src_python)

---

## 1. Document entry

- [Segar Python User Manual](Segar_Python_User_Guide.md)
- [Getting started with Python Topic](Segar_Python_Topic.md)
- [Getting started with Python Service](Segar_Python_Service.md)
- [Getting started with Python Action](Segar_Python_Action.md)
- [Getting started with Python Parameter](Segar_Python_Parameter.md)
- [Python API Reference Manual](Segar_Python_Api_Reference.md)

---

## 2. Directory layout

`src_python` is organized according to the same "project module" layout as `src`, and is installed to:

- `build_x86/output/src_python/topic_example/...`
- `build_x86/output/src_python/service_example/...`
- `build_x86/output/src_python/action_example/...`
- `build_x86/output/src_python/param_example/...`

---

## 3. Build

```bash
cd /home/simon/segar/segar-workspace
./scripts/build_x86.sh
```
---

## 4. Run (example)

### Topic

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```
Open another terminal:

```bash
cd build_x86/output/src_python/topic_example/topic_talker
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
---

## 5. Current module mapping

- `topic_example/topic_talker`
- `topic_example/topic_listener`
- `service_example/service_server`
- `service_example/service_client_sync`
- `service_example/service_client_async`
- `action_example/action_server`
- `action_example/action_client_sync`
- `action_example/action_client_async`
- `param_example/param_server`
- `param_example/param_client`

---

## 6. Operating environment description

`launch.sh` will automatically set:

- `LD_LIBRARY_PATH`
- `SEGAR_PATH`
- `PYTHONPATH` (contains `output/lib/pythonX.Y/site-packages`)

Usually there is no need to manually set dependency paths.

If `plugin not found for package=example` appears in the running log, it means that the current SDK does not yet contain the Python interface plugin of `example` (`segar_py_*plugin_example.so`).
