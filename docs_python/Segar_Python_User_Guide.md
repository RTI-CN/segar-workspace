# Segar Python 用户手册

## 简介

本文档是 Segar Python 版总入口，覆盖最核心通信能力：

- Topic（发布订阅）
- Service（请求响应）
- Action（长时任务）
- Parameter（参数系统）

> 设计原则：Python 接口风格与 Segar C++ 保持一致，统一使用 `Init/CreateNode/WaitForShutdown` 主流程。

---

## 快速开始

### 1. 编译安装

```bash
cd /home/simon/segar/segar-workspace
./scripts/build_x86.sh
```

### 2. 运行一个 Python 示例模块

示例：Topic Listener

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```

再开一个终端运行 Talker：

```bash
cd build_x86/output/src_python/topic_example/topic_talker
./scripts/launch.sh
```

---

## 核心文档导航

- [Python API 参考](Segar_Python_Api_Reference.md)
- [Python Topic 使用入门](Segar_Python_Topic.md)
- [Python Service 使用入门](Segar_Python_Service.md)
- [Python Action 使用入门](Segar_Python_Action.md)
- [Python Parameter 使用入门](Segar_Python_Parameter.md)
- [Python 工程示例总览](Segar_Python_Examples.md)

---

## 与 C++ 手册的对应关系

Python 文档按 C++ 文档同类目拆分，建议并行阅读：

- C++ 总览：`docs/Segar_User_Guide.md`
- Python 总览：`docs_python/Segar_Python_User_Guide.md`

---

## 当前范围说明

- `src_python` 已覆盖 Topic / Service / Action / Parameter 关键示例。
- Component 系列不在本批 Python 文档范围内。
