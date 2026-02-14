# Getting Started with Segar Broadcasting and Recording Packages

> **Note**: Currently, this tool only supports recording and broadcasting of topics, and service/action will be supported in the future.

---

## 1. Record package

Use whitelist mode to record specified topics:

```bash
segar bag record -w /topic/chatter /topic/image
```

Example output:

```text
[RUNNING]  Record Time: 1770792437    Progress: 2 topics, 45 messages^C
```

Press Ctrl+C to stop recording.

---

## 2. View package information

```bash
segar bag info 20260211144712_0001.record
```

Example output:

```text
record_file:    /home/zz/code/segar2/20260211144712_0001.record
version:        1.0
duration:       4.001878 Seconds
begin_time:     2026-02-11-14:47:13
end_time:       2026-02-11-14:47:17
size:           4080 Bytes (3.984375 KB)
is_complete:    true
message_number: 46
topic_number:   2
topic_info:
                /topic/chatter                                           5 messages: example::msg::String
                /topic/image                                            41 messages: example::msg::Image
```

---

## 3. Broadcast package

### 3.1 Play all topics

```bash
segar bag play -f 20260211144712_0001.record
```

Example output:

```text
file: /home/zz/code/segar2/20260211144712_0001.record, chunk_number: 1, begin_time: 1770792433395624601 (2026-02-11-14:47:13), end_time: 1770792437397502366 (2026-02-11-14:47:17), message_number: 46
earliest_begin_time: 1770792433395624601, latest_end_time: 1770792437397502366, total_msg_num: 46

Please wait 3 second(s) for loading...
Hit Ctrl+C to stop, Space to pause, or 's' to step.

[RUNNING] Record Time: 1770792437.398    Progress: 4.002 / 4.002
play finished.
```

### 3.2 Play the specified topic

Use `-c` to specify the topic to play:

```bash
segar bag play -c /topic/chatter /topic/image -f 20260211144712_0001.record
```

Example output:

```text
file: /home/zz/code/segar2/20260211144712_0001.record, chunk_number: 1, begin_time: 1770792433395624601 (2026-02-11-14:47:13), end_time: 1770792437397502366 (2026-02-11-14:47:17), message_number: 46
earliest_begin_time: 1770792433395624601, latest_end_time: 1770792437397502366, total_msg_num: 46

Please wait 3 second(s) for loading...
Hit Ctrl+C to stop, Space to pause, or 's' to step.

[RUNNING] Record Time: 1770792437.398    Progress: 4.002 / 4.002
play finished.
```
