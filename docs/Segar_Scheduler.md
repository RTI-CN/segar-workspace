# Getting Started with Segar Scheduler

> **Description**: Scheduling configuration is used to define the CPU affinity, scheduling strategy, and priority of processes/threads/business tasks to achieve CPU resource isolation and real-time guarantee. Loaded via mainboard `-s` parameter, usually used with Component.

---

## 1. Core functions/document specifications

- **Core role**: Define the CPU affinity (core binding), scheduling strategy (real-time/time-sharing), and priority of the process/thread/business task, and achieve CPU resource isolation and real-time guarantee through mainboard `-s` parameter association
- **File Specifications**:
  - Format: Proto text format
  - Naming: `segar_sched.conf` is recommended (or named according to the policy, such as `classic_sched.conf`), which needs to completely match the sched_name specified by mainboard `-s`
  - Storage: It is recommended to place it in the `config/scheduler/` directory. The framework reads the scheduling configuration in the segar.pb.conf directory by default.

---

## 2. Scheduling file field syntax and examples

Follow the Proto text format, and key fields must strictly match the scheduling policy. Example:

```text
# Segar scheduling configuration file
scheduler_conf {
# Global scheduler mode: classic (classic group scheduling), core rules are managed by groups
    policy: "classic"
# Process-level default CPU affinity: all threads are bound to cores 0-7 and 16-23 by default (16 cores in total)
    process_level_cpuset: "0-7,16-23"
# Refined configuration of core threads of the framework (to ensure real-time performance of key threads)
    threads: [
        {
name: "async_log" # Asynchronous log flush thread
cpuset: "1" # Exclusively use core 1 to avoid competition
policy: "SCHED_OTHER" # Ordinary time-sharing scheduling (non-real-time)
prio: 0 # Medium priority (SCHED_OTHER default)
        }, {
name: "shm" # Shared memory communication thread (core real-time thread)
cpuset: "2" # Exclusively use core 2
policy: "SCHED_FIFO" # Real-time first-in-first-out (strong real-time scenario)
prio: 10 # Medium and high real-time priority (1~99)
        }
    ]
#Classic mode grouping configuration (isolating CPU resources by business)
    classic_conf {
        groups: [
            {
name: "group1" # Task group 1: non-exclusive dynamic scheduling
processor_num: 16 # Number of available cores (consistent with cpuset)
affinity: "range" # Dynamic scheduling: tasks float within the core range
cpuset: "0-7,16-23" # Bind cores 0-7 and 16-23
processor_policy: "SCHED_OTHER" # Default time-sharing scheduling for groups
processor_prio: 0 # Default medium priority for grouping
tasks: [ #Business task list
                    {
name: "E" # Task E: Basic business
prio: 0 #Inherit group priority
                    }
                ]
            }, {
name: "group2" # Task group 2: one-to-one exclusive binding
                processor_num: 16
affinity: "1to1" # Exclusive binding: each task fixes one core
cpuset: "8-15,24-31" # Bind cores 8-15 and 24-31 (isolated from group1)
                processor_policy: "SCHED_OTHER"
                processor_prio: 0
                tasks: [
                    {
                        name: "A"
                        prio: 0
                    }, {
                        name: "B"
prio: 1 # slightly higher than A
                    }, {
                        name: "C"
                        prio: 2
                    }, {
                        name: "D"
prio: 3 # The highest in the group
                    }
                ]
            }
        ]
    }
}
```

---

## 3. Scheduling parameters in mainboard

- **--sched_name=sched_name** (`-s`): Specify the scheduling policy configuration file (relative path/absolute path) of the process, such as `segar_sched.conf` or `/home/user/config/segar_sched.conf`
- **--process_group=process_group** (`-p`): Specifies the process namespace in which the component runs. The process_group specified by `-p` must exist in the scheduler file specified by `-s`

Example:

```bash
mainboard -d examples/car_component.dag -p car_component_proc -s high_priority
```

---

## 4. (Optional reading) Advanced instructions

### 4.1 Process namespace and multi-process collaboration

- **Process Namespace (Process Group)**: By specifying different process_groups through `-p`, components can be dispersed to run in different processes to achieve process isolation.
- **Topic communication**: Components of different processes can communicate through the same Topic name (it is necessary to ensure that the Topic has no naming conflicts)
- **Parameter sharing**: Components under the same process_group can share process-level parameters, and different processes need to be specified individually through configuration files.
