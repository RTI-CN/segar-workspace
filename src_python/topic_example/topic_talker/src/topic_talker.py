#!/usr/bin/env python3

import time

from example.msg import String
from segar.python import segar


def main():
    if not segar.Init("topic_talker"):
        return 1
    node = segar.CreateNode("topic_talker")
    if node is None:
        return 1
    writer = node.CreateWriter("/topic/chatter", String)
    if writer is None:
        print("[topic_talker] create writer failed")
        return 1

    seq = 0
    while segar.OK():
        msg = String(data=str(seq))
        if writer.Write(msg):
            print(f"[topic_talker] sent: {msg.data}")
        else:
            print(f"[topic_talker] write failed: {msg.data}")
        seq += 1
        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
