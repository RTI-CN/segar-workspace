#!/usr/bin/env python3

from example.msg import String
from segar.python import segar


def main():
    if not segar.Init("topic_listener"):
        return 1
    node = segar.CreateNode("topic_listener")
    if node is None:
        return 1

    def on_msg(msg):
        print(f"[topic_listener] recv: {msg.data}")

    reader = node.CreateReader("/topic/chatter", String, on_msg)
    if reader is None:
        print("[topic_listener] create reader failed")
        return 1

    print("[topic_listener] waiting for messages...")
    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
