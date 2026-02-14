# Tracing User Guide

This article is for users and covers the enabling, data generation, import and query of tracing.

## 0. Concept overview

- **node**: manage source process/component
- **session_type**: Session type, enumerated as TOPIC/SERVICE/ACTION/PARAM
- **session**: Session name (such as the name of topic/service/action/parameter), used to indicate the specific business of related session_type
- **session_idx**: The unique associated key of a session instance, used to indicate the "session number" of the session
- **stage**: Stage point in the link (such as PUBLISH, MESSAGE_RESTORED, START_CALLBACK)
- **event**: A proprietary field in `session_timeline`, indicating the event point name at that moment; when session_type is topic, it is equivalent to the topic name
- **delay**: The time spent at adjacent points and the cumulative time spent at the starting point
- **join/leave**: node’s online/offline status changes in the topology
- **drop**: Lost statistics caused by matching failure in adjacent stages

## 1. Scope of application and conditions of validity

- **Parameter**: Supported by default
- **Msg/Service/Action**: Non-default, `_enable_tracing_` needs to be added to the `.msg/.srv/.action` file header to take effect
- **Messages without `_enable_tracing_` enabled will not be logged**

Enable example (first line of file header):

```text
_enable_tracing_
int32 exec_times
---
int32 total
ResultDetail detail
---
int32 current
```

## 2. Configuration file

Configuration file path (read during runtime): `$SEGAR_PATH/config/tracing_config.pb.txt`

Key fields:

| Field | illustrate |
|------|------|
| trace_data_path | Data root directory (supports absolute path or `~/`; relative path will be spelled to `$SEGAR_PATH`) |
| max_trace_data_folder_size | Maximum directory size in MB |
| auto_clear_old_trace_data | Whether to clean old directories before each run (default `false`) |

## 3. Start tracing to obtain trace data

After executing `source segar_setup.bash` in the output directory, run:

```bash
mainboard -d config/tracing_node.dag
```

You can start TracingNodeComponent and start collecting trace data.

tracing_node.dag configuration example:

```text
module_config {
    module_library : "third_party/lib/libtracing_node_component.so"
    components {
        component_class_name : "rti::segar::TracingNodeComponent"
        config {
            inner_node_name : "tracing_node"
        }
    }
}
```

**Generated data directory and format**: In the directory specified by `trace_data_path` in `tracing_config.pb.txt`, create the timestamp subdirectory `YYYYMMDDHHMMSS`, which contains:

- a `topo.txt`
- Several `<node_id>_<YYYYMMDDHHMMSS>.tra` files

## 4. Import database

### 4.1 First use: Initialization environment (root/sudo required)

```bash
sudo tracing -i
```

MySQL, python3-pymysql will be installed, and root authentication will be repaired so that ordinary users can connect.

### 4.2 Import tracing data

```bash
tracing -d /path/to/trace_data/20240206123000
```

Append import (not commonly used, does not clear the database):

```bash
tracing -d /path/to/trace_data/20240206123000 -a
```

The above command needs to be executed in the output directory after `source segar_setup.bash`.

## 5. How to use tracing query

### 5.1 Using help

```bash
tracing -c "help"
```

Example output:

```bash
$ tracing -c "help"
+----------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| command                                                                                      | description                                                                                              |
+----------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------+
| nodes                                                                                        | Query all nodes. Returns: node_name, node_id, status, first_join_time, last_leave_time.                  |
| sessions                                                                                     | Query all sessions. Returns: session_type, session.                                                      |
| stages                                                                                       | Query stage definitions. Use this to confirm valid stage values.                                         |
| sessions_of_node(node_name)                                                                  | Query sessions of the specified node. Returns: session_type, session.                                    |
| nodes_of_session(session_type, session)                                                      | Query nodes of the specified session. Params: session_type, session.                                     |
| session_summary_by_start_node(session_type, start_node, session_name, filter_sql, limit_num) | Summary list filtered by start_node/session_name and optional SQL predicate.                             |
| session_summary_all(session_type, filter_sql, limit_num)                                     | Summary list with optional SQL predicate; session_type supports TOPIC/SERVICE/ACTION/PARAM (or 0/1/2/3). |
| session_timeline(session_type, session_idx)                                                  | Full timeline for one session; session_type supports TOPIC/SERVICE/ACTION/PARAM (or 0/1/2/3).            |
| query_delay_same_topic(node_1st, topic, node_2nd, filter_sql, limit_num)                     | Same-topic timeline with optional SQL predicate.                                                         |
| query_delay_cross_topic(node_1st, topic_1st, node_2nd, topic_2nd, filter_sql, limit_num)     | Cross-topic timeline with optional SQL predicate.                                                        |
| drop_info(node_1st, topic, node_2nd, filter_sql)                                             | Query drop statistics along fixed adjacent pipeline with optional SQL predicate.                         |
| dropped_records(node_1st, stage_1st, topic, node_2nd, stage_2nd, filter_sql, limit_num)      | Dropped records for one stage pair in the same topic with optional SQL predicate.                        |
| rate(node, stage, topic, filter_sql, limit_num)                                              | Per-second rate (pps) for node/topic/stage with optional SQL predicate.                                  |
+----------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------+
13 rows in set.
OK: 0 rows affected.
```

### 5.2 Basic query

```bash
tracing -c "nodes"
tracing -c "sessions"
tracing -c "stages"
tracing -c "sessions_of_node('node_name')"
tracing -c "nodes_of_session('session_type', 'session')"
```

Example:

```bash
$ tracing -c "nodes"
+--------------+----------------------+--------+----------------------------+----------------------------+
| node_name    | node_id              | status | first_join_time            | last_leave_time            |
+--------------+----------------------+--------+----------------------------+----------------------------+
| CLI_1636415  | 3121243826755017703  | LEFT   | 2026/02/11 19:12:39.395177 | 2026/02/11 19:13:41.223889 |
| param_timer  | 15565186663480987662 | LEFT   | 2026/02/11 19:12:39.395024 | 2026/02/11 19:13:41.223907 |
| sensor1      | 6910855445724205230  | LEFT   | 2026/02/11 19:12:38.385251 | 2026/02/11 19:13:41.207730 |
| sensor10     | 15644514452393993644 | LEFT   | 2026/02/11 19:12:39.293679 | 2026/02/11 19:13:41.177507 |
| sensor11     | 13784232787874767474 | JOINED | 2026/02/11 19:12:38.459820 |                            |
| sensor12     | 1709364246235240553  | LEFT   | 2026/02/11 19:12:38.464828 | 2026/02/11 19:13:41.211200 |
| sensor13     | 713371279686677275   | LEFT   | 2026/02/11 19:12:38.482725 | 2026/02/11 19:13:41.214352 |
| tracing_node | 1296936435589820685  | LEFT   | 2026/02/11 19:12:38.189976 | 2026/02/11 19:13:41.221601 |
......
+--------------+----------------------+--------+----------------------------+----------------------------+
31 rows in set.
OK: 0 rows affected.

$ tracing -c "sessions"
+--------------+-----------------------------+
| session_type | session                     |
+--------------+-----------------------------+
| ACTION       | demo_action                 |
| PARAMETER    | node_a/parameter_serv       |
| PARAMETER    | param_timer/parameter_serv  |
| PARAMETER    | sensor1/parameter_serv      |
| SERVICE      | add_two_ints                |
| TOPIC        | sensor_topic_1              |
| TOPIC        | sensor_topic_10             |
......
+--------------+-----------------------------+

$ tracing -c "sessions_of_node('node_a')"
+--------------+-----------------------+
| session_type | session               |
+--------------+-----------------------+
| PARAMETER    | node_a/parameter_serv |
| SERVICE      | add_two_ints          |
| TOPIC        | NodeA                 |
| TOPIC        | sensor_topic_1        |
| TOPIC        | sensor_topic_4        |
| TOPIC        | sensor_topic_6        |
| TOPIC        | sensor_topic_7        |
+--------------+-----------------------+
7 rows in set.
OK: 0 rows affected.

$ tracing -c "nodes_of_session('topic', 'NodeA')"
+-----------+---------------------+
| node_name | node_id             |
+-----------+---------------------+
| node_a    | 8899687290348478343 |
| node_b    | 984110685106040982  |
| node_c    | 777002420257141533  |
| node_h    | 530094450857284016  |
+-----------+---------------------+
4 rows in set.
OK: 0 rows affected.
```

### 5.3 Session query

```bash
tracing -c "session_summary_by_start_node('ACTION', 'node_f', 'demo_action', 'start_time>''2026/02/11 19:13:39.530409''', 100)"
tracing -c "session_summary_all('ACTION', '', 100)"
tracing -c "session_timeline('ACTION', 7027348180303884)"
```

Example:

```bash
$ tracing -c "session_summary_by_start_node('ACTION', 'node_f', 'demo_action', 'start_time>''2026/02/11 19:13:38.530409''', 100)"
+------------------+----------------------------+----------------------------+-------------+--------+------------+-------------+-----------------------+------------------------+
| session_idx      | start_time                 | end_time                   | delay_total | points | node_count | topic_count | start_callback_points | finish_callback_points |
+------------------+----------------------------+----------------------------+-------------+--------+------------+-------------+-----------------------+------------------------+
| 7027348180303950 | 2026/02/11 19:13:39.530409 | 2026/02/11 19:13:39.531471 | 1062        | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303951 | 2026/02/11 19:13:39.531602 | 2026/02/11 19:13:39.532852 | 1250        | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303952 | 2026/02/11 19:13:40.780358 | 2026/02/11 19:13:40.781647 | 1289        | 20     | 2          | 4           | 4                     | 4                      |
+------------------+----------------------------+----------------------------+-------------+--------+------------+-------------+-----------------------+------------------------+
3 rows in set.
OK: 0 rows affected.

$ tracing -c "session_summary_all('ACTION', '', 5)"
+------------------+----------------------------+----------------------------+------------+--------------+-------------+--------+------------+-------------+-----------------------+------------------------+
| session_idx      | start_time                 | end_time                   | start_node | session_name | delay_total | points | node_count | topic_count | start_callback_points | finish_callback_points |
+------------------+----------------------------+----------------------------+------------+--------------+-------------+--------+------------+-------------+-----------------------+------------------------+
| 7027348180303877 | 2026/02/11 19:12:43.280391 | 2026/02/11 19:12:43.282086 | node_f     | demo_action  | 1695        | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303878 | 2026/02/11 19:12:43.530322 | 2026/02/11 19:12:43.531309 | node_f     | demo_action  | 987         | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303879 | 2026/02/11 19:12:44.530422 | 2026/02/11 19:12:44.531996 | node_f     | demo_action  | 1574        | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303880 | 2026/02/11 19:12:45.530326 | 2026/02/11 19:12:45.531332 | node_f     | demo_action  | 1006        | 20     | 2          | 4           | 4                     | 4                      |
| 7027348180303881 | 2026/02/11 19:12:45.780415 | 2026/02/11 19:12:45.781995 | node_f     | demo_action  | 1580        | 20     | 2          | 4           | 4                     | 4                      |
+------------------+----------------------------+----------------------------+------------+--------------+-------------+--------+------------+-------------+-----------------------+------------------------+
5 rows in set.
OK: 0 rows affected.

$ tracing -c "session_timeline('ACTION', 7027348180303884)"
+-----------+-------------------+------------------+-----+-----------------+------------------+
| node_name | event             | stage            | seq | delay_with_prev | delay_from_start |
+-----------+-------------------+------------------+-----+-----------------+------------------+
| node_f    | send_goalRequest  | PUBLISH          | 196 | 0               | 0                |
| node_f    | send_goalRequest  | PUBLISH_FINISHED | 196 | 19              | 19               |
| node_g    | send_goalRequest  | MESSAGE_RESTORED | 196 | 99              | 118              |
| node_g    | send_goalRequest  | START_CALLBACK   | 196 | 37              | 155              |
| node_g    | send_goalRequest  | FINISH_CALLBACK  | 196 | 288             | 443              |
| node_g    | send_goalRequest  | PUBLISH          | 36  | 2               | 445              |
| node_g    | send_goalRequest  | PUBLISH_FINISHED | 36  | 16              | 461              |
| node_f    | send_goalReply    | MESSAGE_RESTORED | 36  | 90              | 551              |
| node_f    | send_goalReply    | START_CALLBACK   | 36  | 12              | 563              |
| node_f    | send_goalReply    | FINISH_CALLBACK  | 36  | 24              | 587              |
| node_f    | get_resultRequest | PUBLISH          | 197 | 10              | 597              |
| node_f    | get_resultRequest | PUBLISH_FINISHED | 197 | 15              | 612              |
| node_g    | get_resultRequest | MESSAGE_RESTORED | 197 | 153             | 765              |
| node_g    | get_resultRequest | START_CALLBACK   | 197 | 27              | 792              |
| node_g    | get_resultRequest | FINISH_CALLBACK  | 197 | 4               | 796              |
| node_g    | get_resultRequest | PUBLISH          | 37  | 1               | 797              |
| node_g    | get_resultRequest | PUBLISH_FINISHED | 37  | 17              | 814              |
| node_f    | get_resultReply   | MESSAGE_RESTORED | 37  | 184             | 998              |
| node_f    | get_resultReply   | START_CALLBACK   | 37  | 11              | 1009             |
| node_f    | get_resultReply   | FINISH_CALLBACK  | 37  | 15              | 1024             |
+-----------+-------------------+------------------+-----+-----------------+------------------+
20 rows in set.
OK: 0 rows affected.
```

### 5.4 Delay/Packet Loss/Rate

```bash
tracing -c "query_delay_same_topic('node_a', 'NodeA', 'node_h', '', 100)"
tracing -c "query_delay_cross_topic('node_a', 'topic_a', 'node_c', 'topic_b', '', 100)"
tracing -c "drop_info('node_a', 'NodeA', 'node_h', '')"
tracing -c "dropped_records('node_a', 3, 'NodeA', 'node_h', 21, '', 100)"
tracing -c "rate('node_a', 2, 'NodeA', 'time>''2026/02/11 19:13:05'' and time<''2026/02/11 19:13:10''', 100)"
```

Example:

```bash
$ tracing -c "query_delay_same_topic('node_a', 'NodeA', 'node_h', '', 10)"
+------------------+-----+----------------------------+---------------------------------+---------------------------------+-------------------------------+--------------------------------+-------------+
| session_idx      | seq | publish_time               | delay_publish_finished_with_pre | delay_message_restored_with_pre | delay_start_callback_with_pre | delay_finish_callback_with_pre | delay_total |
+------------------+-----+----------------------------+---------------------------------+---------------------------------+-------------------------------+--------------------------------+-------------+
| 7027223626252289 | 1   | 2026/02/11 19:12:39.259376 | 102                             | 0                               | 0                             | 0                              | 0           |
| 7027223626252290 | 2   | 2026/02/11 19:12:39.309675 | 174                             | 0                               | 0                             | 0                              | 0           |
| 7027223626252291 | 3   | 2026/02/11 19:12:39.359337 | 54                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252292 | 4   | 2026/02/11 19:12:39.409315 | 45                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252293 | 5   | 2026/02/11 19:12:39.459386 | 50                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252294 | 6   | 2026/02/11 19:12:39.509365 | 54                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252295 | 7   | 2026/02/11 19:12:39.559376 | 52                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252296 | 8   | 2026/02/11 19:12:39.609295 | 52                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252297 | 9   | 2026/02/11 19:12:39.659326 | 96                              | 0                               | 0                             | 0                              | 0           |
| 7027223626252298 | 10  | 2026/02/11 19:12:39.710136 | 49                              | 0                               | 0                             | 0                              | 0           |
+------------------+-----+----------------------------+---------------------------------+---------------------------------+-------------------------------+--------------------------------+-------------+
10 rows in set.
OK: 0 rows affected.

$ tracing -c "drop_info('node_a', 'NodeA', 'node_h', '')"
+-----------+------------------+-----------+------------------+-------------+------------+----------+-----------+
| stage_1st | stage_name_1st   | stage_2nd | stage_name_2nd   | count_front | count_back | drop_num | drop_rate |
+-----------+------------------+-----------+------------------+-------------+------------+----------+-----------+
| 1         | PUBLISH          | 2         | PUBLISH_FINISHED | 1239        | 1239       | 0        | 0.0000    |
| 2         | PUBLISH_FINISHED | 3         | MESSAGE_RESTORED | 1239        | 1162       | 77       | 0.0621    |
| 3         | MESSAGE_RESTORED | 4         | START_CALLBACK   | 1162        | 1162       | 0        | 0.0000    |
| 4         | START_CALLBACK   | 5         | FINISH_CALLBACK  | 1162        | 1162       | 0        | 0.0000    |
+-----------+------------------+-----------+------------------+-------------+------------+----------+-----------
4 rows in set.
OK: 0 rows affected.

$ tracing -c "rate('node_a', 2, 'NodeA', 'time>''2026/02/11 19:13:05'' and time<''2026/02/11 19:13:10''', 100)"
+---------------------+-------------+
| time                | frame_count |
+---------------------+-------------+
| 2026/02/11 19:13:06 | 20          |
| 2026/02/11 19:13:07 | 20          |
| 2026/02/11 19:13:08 | 20          |
| 2026/02/11 19:13:09 | 20          |
+---------------------+-------------+
4 rows in set.
OK: 0 rows affected.
```

## 6. FAQ

- **Only topo.txt is generated, no .tra data file**: Check whether the message definition `_enable_tracing_` is enabled (msg/service/action is not enabled by default); check whether TracingNodeComponent has been started (whether tracing_node.dag has been loaded).
- **trace_data_path does not exist or is empty**: tracing_node is not started or fails to start; or the current user of the directory specified by `trace_data_path` in `tracing_config.pb.txt` does not have write permissions.
