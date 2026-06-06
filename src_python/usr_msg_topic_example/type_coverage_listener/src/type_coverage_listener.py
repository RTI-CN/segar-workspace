#!/usr/bin/env python3

from segar.python import segar
from test_msgs.msg import TypeCoverage


TOPIC = "/topic/usr_msg/type_coverage"


def main():
    if not segar.Init("py_usr_msg_type_coverage_listener"):
        return 1
    node = segar.CreateNode("py_usr_msg_type_coverage_listener")
    if node is None:
        return 1

    def on_msg(msg):
        print(
            "[py_usr_msg_type_coverage_listener] recv: "
            f"int32={msg.int32_value}, note={msg.note}, "
            f"py_bytes={list(msg.py_bytes)}, "
            f"py_bytearray={list(msg.py_bytearray)}, "
            f"py_memoryview={list(msg.py_memoryview)}, "
            f"fixed_bytes={list(msg.fixed_bytes)}, "
            f"samples={len(msg.samples)}, "
            f"detail=({msg.detail.detail_id}, {msg.detail.description})"
        )

    reader = node.CreateReader(TOPIC, TypeCoverage, on_msg)
    if reader is None:
        print("[py_usr_msg_type_coverage_listener] create reader failed")
        return 1

    print("[py_usr_msg_type_coverage_listener] waiting for TypeCoverage messages...")
    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
