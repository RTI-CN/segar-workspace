# Segar Bag User Manual

`segar bag` is used to record, playback and view topic data, based on MCAP format. This document introduces the usage of three subcommands: `record`, `play`, and `info`.

> In order to facilitate the migration of ros2 users, the `segar bag` command style is basically the same as `ros2 bag`

---

## 1. record (recording)

Record the data of the specified topic to the MCAP file.

### Command format

```bash
segar bag record [options]
```
### Required conditions (choose one of two)

| Options | Description |
|------|------|
| `-a, --all` | Record all topics |
| `-t, --topics <topic>...` | Record the specified topic (multiple ones can be specified). Supports `topic:=type` format and also specifies message type (for ROS2, pre-subscription when there is no publisher) |

### Optional parameters

| Options | Description |
|------|------|
| `-o, --output <dir>` | Output directory name, default `segar_bag_{timestamp}/` |
| `-i, --segment-interval <seconds>` | Segment by time, generate new files every N seconds |
| `-m, --segment-size <MB>` | Segment by size, generate new files every N MB |
| `-h, --help` | Show help |

### Output description

The recording results are output to the directory, and the files in the directory are named `{directory name}_0.mcap`, `{directory name}_1.mcap`, etc.

### Example

```bash
# Record the specified topic and output it to my_bag/
segar bag record -t rt/topic/chatter_idl -o my_bag

# Record multiple topics
segar bag record -t /topic_a /topic_b -o multi_bag

# Record all topics and use the default output directory
segar bag record -a

# Use topic:=type to specify the message type (mainly used for recording ROS2 topic)
segar bag record -t rt/chatter:=std_msgs::msg::String -o ros2_bag
# Segment by 100MB
segar bag record -t /chatter -o big_bag -m 100
```
### topic:=type format description

When recording a ROS2 topic, use `-t topic:=type` to specify both the topic name and the message type, for example:

- `-t rt/chatter:=std_msgs::msg::String`: record ROS2 `/chatter` bridge topic
---

## 2. play (playback)

Play back MCAP data from a recording file or directory.

### Command format

```bash
segar bag play <bag_path> [options]
```
### Required parameters

| Parameters | Description |
|------|------|
| `bag_path` | Recording file path or directory path (positional parameter) |

Supports single file (such as `my_bag_0.mcap`) or directory (such as `my_bag/`, automatically expands `*_0.mcap`, `*_1.mcap`, etc.).

### Optional parameters

| Options | Description |
|------|------|
| `-a, --all` | Replay all topics (default behavior) |
| `-t, --topics <topic>...` | Play back only specified topics |
| `-k, --black-topic <name>` | Exclude specified topics |
| `-l, --loop` | Loop playback |
| `-r, --rate <rate>` | Playback rate, default 1.0 |
| `-b, --begin <time>` | Start time (such as 2018-07-01-00:00:00) |
| `-e, --end <time>` | end time |
| `-s, --start <seconds>` | Start from Nth second |
| `-d, --latency <seconds>` | Delay in seconds before starting |
| `-p, --preload <seconds>` | Preload duration |
| `-m, --remap <old:=new>` | Topic remapping |
| `-h, --help` | Show help |

### Example

```bash
# Playback directory
segar bag play my_bag

# Play back a single file
segar bag play my_bag/my_bag_0.mcap

# Only play back the specified topic, loop, 2x speed
segar bag play my_bag -t rt/topic/chatter_idl -l -r 2.0

# Topic remapping: publish /chatter to /remapped_chatter
segar bag play my_bag -m /chatter:=/remapped_chatter
```
---

## 3. info (view information)

View metadata and statistics for a recording file or directory.

### Command format

```bash
segar bag info <path>
```
### Parameters

| Parameters | Description |
|------|------|
| `path` | Recording file or directory path |

When a directory is passed in, the statistics of all `*_N.mcap` files in the directory will be summarized.

### Output description

- `record_file`: path
- `version`: version
- `duration`: duration (seconds)
- `begin_time` / `end_time`: start and end time
- `size`: file size
- `is_complete`: whether it is complete
- `message_number`: total number of messages
- `topic_number`: topic number
- `topic_info`: number and type of messages for each topic

### Example

```bash
# View single file
segar bag info my_bag_0.mcap

# View directory (summary statistics)
segar bag info my_bag
```
---

## 4. Complete example: recording and playback ROS2 demo_nodes_cpp topic

> The MCAP recorded by segar bag is in a standard format and can be played and viewed using [Foxglove Studio](https://foxglove.dev/) and `ros2 bag`.

The following takes the `demo_nodes_cpp` that comes with ROS 2 as an example to demonstrate how to use segar bag to record a ROS2 topic and play it back using segar bag and ros2 bag respectively.

### 1. Recording

**Terminal 1**: Start ROS2 talker

```bash
ros2 run demo_nodes_cpp talker
```
**Terminal 2**: Use segar bag to record `/chatter` (the topic name is `rt/chatter` after bridging)

```bash
segar bag record -a -t rt/chatter:=std_msgs::msg::String -o ros2_demo_bag
```After recording is completed, press Ctrl+C to stop. The output directory is `ros2_demo_bag/` and the file is `ros2_demo_bag_0.mcap`.

### 2. Use segar bag to play back

```bash
segar bag play ros2_demo_bag
# Or specify a single file
segar bag play ros2_demo_bag/ros2_demo_bag_0.mcap
```
**Terminal 3** (optional): Start listener to receive playback data

```bash
ros2 run demo_nodes_cpp listener
```
### 3. Use ros2 bag to play back

MCAP files generated by segar bag can be played back directly using ros2 bag. The topic name on the ROS2 side is `/chatter`, and the recorded `rt/chatter` needs to be remapped:

```bash
ros2 bag play ros2_demo_bag/ros2_demo_bag_0.mcap --remap rt/chatter:=/chatter
```
> Note: `rt/chatter` is the topic name after segar bridging, `/chatter` is the ROS2 native topic name. After remapping through `--remap`, `ros2 run demo_nodes_cpp listener` can receive normally.
