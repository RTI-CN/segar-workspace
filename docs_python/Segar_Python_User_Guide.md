# Segar Python User Manual

## Introduction

This document is the general entrance to the Segar Python version, covering the core communication capabilities:

- Topic (publish and subscribe)
- Service (request response)
- Action (long-term task)
- Parameter (parameter system)

> Design principle: The Python interface style is consistent with Segar C++, and the `Init/CreateNode/WaitForShutdown` main process is uniformly used.

---

## Quick start

### 1. Compile and install

```bash
cd /home/simon/segar/segar-workspace
./scripts/build_x86.sh
```
### 2. Run a Python example module

Example: Topic Listener

```bash
cd build_x86/output/src_python/topic_example/topic_listener
./scripts/launch.sh
```
Open another terminal and run Talker:

```bash
cd build_x86/output/src_python/topic_example/topic_talker
./scripts/launch.sh
```
---

## Core document navigation

- [Python API Reference](Segar_Python_Api_Reference.md)
- [Getting started with Python Topic](Segar_Python_Topic.md)
- [Getting started with Python Service](Segar_Python_Service.md)
- [Getting started with Python Action](Segar_Python_Action.md)
- [Getting started with Python Parameter](Segar_Python_Parameter.md)
- [Python project examples overview](Segar_Python_Examples.md)

---

## Correspondence to C++ manual

Python documentation is split into the same categories as C++ documentation, and it is recommended to read them in parallel:

- C++ Overview: `docs/Segar_User_Guide.md`
- Python overview: `docs_python/Segar_Python_User_Guide.md`

---

## Current scope description

- `src_python` has covered Topic / Service / Action / Parameter key examples.
- The Component family is outside the scope of this batch of Python documentation.
