# Getting Started with Segar CLI

> **Note**: For the convenience of users, the usage and output style of this tool are basically the same as ROS 2. The output information has been partially expanded and command completion is supported.
>
> **Environment preparation**: You must complete the environment variable settings before using the following commands. Execute `source segar_setup.bash` in the output directory of the compiled product, or see the project README for running instructions.

---

## 1. node subcommand

```bash
segar node
usage: segar node [-h] {list,info}...

Various node related utilities

commands:
  list                        List all nodes
  info <NodeName>             Node information
```

---

## 2. topic subcommand

```bash
segar topic
usage: segar topic [-h] {list,info,bw,hz,type,echo} ...

Various topic related utilities

commands:
  list                        List all topics
  info <TopicName>            Print information about a topic
  bw <TopicName>              Display bandwidth of a topic
  hz <TopicName>              Display publishing rate of topic
  type <TopicName>            Display type of topic
  echo <TopicName>            Echo messages from a topic (protobuf only)
```

---

## 3. service subcommand

```bash
segar service
usage: segar service [-h] {list,type,info} ...

Various service related utilities

commands:
  list                        List all services
  type <ServiceName>          Display .srv type for a service
  info <ServiceName>          Display info of service
```

---

## 4. action subcommand

```bash
segar action
usage: segar action [-h] {list,type,info} ...

Action related utilities

commands:
  list                        List all actions
  type <ActionName>           Display .action type for an action
  info <ActionName>           Display info of action
```

---

## 5. param subcommand

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