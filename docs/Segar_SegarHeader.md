# SegarHeader automatic injection configuration instructions

> **Note**: Whether SegarHeader is automatically injected is controlled by the `type_src/transport_messages` list.

---

## 1. Background and goals

- `SegarHeader` is used to carry transmission side element information such as session/sequence number/timestamp etc.
- Only messages sent and received through the Segar API need to automatically add `SegarHeader`
- In order to avoid coupling between business message definition and injection strategy, change to "configuration-driven" mode

---

## 2. Configure entry

Create files under each message root directory (`TYPE_SRC_DIR`):

```text
type_src/transport_messages
```
- One rule per line
- The path is relative to the `type_src` directory
- Supports comment lines (starting with `#`) and blank lines

---

## 3. Rule format

### 3.1 File-level rules (exact match)

```text
example/msg/Image.msg
example/srv/SetCameraInfo.srv
example/action/LookUpTransform.action
```
### 3.2 Wildcard rules

```text
example/msg/Test*
example/action/*
```
Description: Matching uses `fnmatch` semantics (`*`, `?`, `[]`), where `directory/*` matches interface definitions in that directory and its subdirectories.

---

## 4. Sample configuration

```text
# Only inject SegarHeader into these three types of interfaces
test_msgs/action/*
test_msgs/msg/Test*
test_msgs/srv/*
```
Meaning:

- `test_msgs/action/*`: all `.action` injections in this directory and subdirectories
- `test_msgs/msg/Test*`: only match message definitions starting with `Test`
- `test_msgs/srv/*`: all `.srv` injections in this directory and subdirectories

---

## 5. FAQ

### 5.1 Configured but not effective

- Check if `transport_messages` is placed in the `TYPE_SRC_DIR` root directory
- Check if path is "relative `type_src`" instead of absolute path
- Check if the build uses the new version of `msg_tool`
- Clean old build artifacts and regenerate

### 5.2 The rule writes the directory but does not match it

- The directory itself is not a legal rule, please use wildcard writing: `pkg/msg/*`, `pkg/srv/*`, `pkg/action/*`
- Avoid extra spaces before and after rules
